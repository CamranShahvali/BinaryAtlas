#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "binaryatlas/core/analyzer.hpp"
#include "binaryatlas/graph/dot_exporter.hpp"
#include "binaryatlas/output/json_writer.hpp"
#include "binaryatlas/output/terminal_renderer.hpp"
#include "binaryatlas/util/file_io.hpp"
#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{
namespace
{

struct CliOptions
{
  std::string command;
  std::filesystem::path binary_path;
  std::optional<std::string> section_name;
  std::optional<Address> start_address;
  std::optional<Address> end_address;
  std::optional<std::string> function_selector;
  std::optional<std::filesystem::path> json_path;
  std::optional<std::filesystem::path> dot_path;
  std::optional<std::filesystem::path> dot_directory;
  bool json_stdout {false};
  bool verbose {false};
  bool full {false};
};

[[nodiscard]] std::optional<Address> parseAddress(std::string_view value)
{
  try
  {
    std::size_t consumed = 0;
    const std::string text(value);
    const int base = text.starts_with("0x") || text.starts_with("0X") ? 16 : 10;
    const auto parsed = std::stoull(text, &consumed, base);
    if (consumed != text.size())
    {
      return std::nullopt;
    }
    return static_cast<Address>(parsed);
  }
  catch (...)
  {
    return std::nullopt;
  }
}

[[nodiscard]] bool isFlag(std::string_view value)
{
  return value.starts_with("--");
}

void printUsage()
{
  std::cerr << "BinaryAtlas\n"
            << "Usage:\n"
            << "  binaryatlas inspect <binary> [--json [path]] [--verbose]\n"
            << "  binaryatlas disasm <binary> [--section name] [--start addr] [--end addr] [--json [path]]\n"
            << "  binaryatlas functions <binary> [--json [path]] [--verbose]\n"
            << "  binaryatlas cfg <binary> --function <addr|name> [--dot path] [--json [path]]\n"
            << "  binaryatlas callgraph <binary> [--dot path] [--json [path]]\n"
            << "  binaryatlas heuristics <binary> [--json [path]] [--verbose]\n"
            << "  binaryatlas analyze <binary> [--full] [--dot-dir dir] [--json [path]] [--verbose]\n";
}

[[nodiscard]] std::optional<CliOptions> parseCli(int argc, char** argv)
{
  if (argc < 3)
  {
    printUsage();
    return std::nullopt;
  }

  CliOptions options;
  options.command = argv[1];
  options.binary_path = argv[2];

  for (int index = 3; index < argc; ++index)
  {
    const std::string_view argument = argv[index];
    if (argument == "--json")
    {
      if (index + 1 < argc && !isFlag(argv[index + 1]))
      {
        options.json_path = argv[++index];
      }
      else
      {
        options.json_stdout = true;
      }
      continue;
    }
    if (argument == "--dot")
    {
      if (index + 1 >= argc || isFlag(argv[index + 1]))
      {
        std::cerr << "--dot requires a path\n";
        return std::nullopt;
      }
      options.dot_path = argv[++index];
      continue;
    }
    if (argument == "--dot-dir")
    {
      if (index + 1 >= argc || isFlag(argv[index + 1]))
      {
        std::cerr << "--dot-dir requires a directory\n";
        return std::nullopt;
      }
      options.dot_directory = argv[++index];
      continue;
    }
    if (argument == "--section")
    {
      if (index + 1 >= argc || isFlag(argv[index + 1]))
      {
        std::cerr << "--section requires a value\n";
        return std::nullopt;
      }
      options.section_name = argv[++index];
      continue;
    }
    if (argument == "--start")
    {
      if (index + 1 >= argc)
      {
        return std::nullopt;
      }
      options.start_address = parseAddress(argv[++index]);
      if (!options.start_address.has_value())
      {
        std::cerr << "invalid --start address\n";
        return std::nullopt;
      }
      continue;
    }
    if (argument == "--end")
    {
      if (index + 1 >= argc)
      {
        return std::nullopt;
      }
      options.end_address = parseAddress(argv[++index]);
      if (!options.end_address.has_value())
      {
        std::cerr << "invalid --end address\n";
        return std::nullopt;
      }
      continue;
    }
    if (argument == "--function")
    {
      if (index + 1 >= argc || isFlag(argv[index + 1]))
      {
        std::cerr << "--function requires an address or name\n";
        return std::nullopt;
      }
      options.function_selector = argv[++index];
      continue;
    }
    if (argument == "--verbose")
    {
      options.verbose = true;
      continue;
    }
    if (argument == "--full")
    {
      options.full = true;
      continue;
    }

    std::cerr << "unknown option: " << argument << '\n';
    return std::nullopt;
  }

  return options;
}

template <typename Renderer>
int emit(
    const CliOptions& options,
    Renderer&& renderer,
    std::string_view default_terminal_output)
{
  const std::string rendered = renderer();
  if (options.json_stdout)
  {
    std::cout << rendered;
    return EXIT_SUCCESS;
  }

  if (options.json_path.has_value())
  {
    if (const Status status = writeTextFile(*options.json_path, rendered); !status)
    {
      std::cerr << status.error().message << '\n';
      return EXIT_FAILURE;
    }
    std::cout << "wrote " << options.json_path->string() << '\n';
    return EXIT_SUCCESS;
  }

  std::cout << default_terminal_output;
  return EXIT_SUCCESS;
}

[[nodiscard]] const RecoveredFunction* resolveFunction(
    const ProgramAnalysis& analysis,
    const std::string& selector)
{
  if (const std::optional<Address> address = parseAddress(selector))
  {
    const auto iterator = std::find_if(
        analysis.functions.begin(),
        analysis.functions.end(),
        [address](const RecoveredFunction& function) { return function.entry == *address; });
    return iterator == analysis.functions.end() ? nullptr : &*iterator;
  }

  const auto iterator = std::find_if(
      analysis.functions.begin(),
      analysis.functions.end(),
      [&selector](const RecoveredFunction& function) { return function.name == selector; });
  return iterator == analysis.functions.end() ? nullptr : &*iterator;
}

[[nodiscard]] std::string sanitizeName(std::string name)
{
  for (char& character : name)
  {
    if (!(std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_'))
    {
      character = '_';
    }
  }
  return name;
}

}  // namespace
}  // namespace binaryatlas

int main(int argc, char** argv)
{
  using namespace binaryatlas;

  const std::optional<CliOptions> options = parseCli(argc, argv);
  if (!options.has_value())
  {
    return EXIT_FAILURE;
  }

  BinaryAtlasEngine engine;

  if (options->command == "inspect")
  {
    const Result<BinaryImage> image = engine.load(options->binary_path);
    if (!image)
    {
      std::cerr << image.error().message << '\n';
      return EXIT_FAILURE;
    }

    return emit(
        *options,
        [&]() { return JsonWriter::renderInspect(image.value()); },
        TerminalRenderer::renderInspect(image.value(), options->verbose));
  }

  if (options->command == "disasm")
  {
    DisassemblyOptions disassembly_options;
    disassembly_options.section_name = options->section_name;
    disassembly_options.start_address = options->start_address;
    disassembly_options.end_address = options->end_address;

    const Result<DisassemblyResult> disassembly =
        engine.disassemble(options->binary_path, disassembly_options);
    if (!disassembly)
    {
      std::cerr << disassembly.error().message << '\n';
      return EXIT_FAILURE;
    }

    return emit(
        *options,
        [&]() { return JsonWriter::renderDisassembly(disassembly.value()); },
        TerminalRenderer::renderDisassembly(disassembly.value(), options->verbose));
  }

  if (options->command == "functions" || options->command == "cfg" ||
      options->command == "callgraph" || options->command == "heuristics" ||
      options->command == "analyze")
  {
    const Result<AnalysisBundle> bundle = engine.analyze(options->binary_path);
    if (!bundle)
    {
      std::cerr << bundle.error().message << '\n';
      return EXIT_FAILURE;
    }
    const AnalysisBundle& analysis_bundle = bundle.value();

    if (options->command == "functions")
    {
      return emit(
          *options,
          [&]() { return JsonWriter::renderFunctions(analysis_bundle.analysis); },
          TerminalRenderer::renderFunctions(analysis_bundle.analysis, options->verbose));
    }

    if (options->command == "cfg")
    {
      if (!options->function_selector.has_value())
      {
        std::cerr << "cfg requires --function\n";
        return EXIT_FAILURE;
      }
      const RecoveredFunction* function =
          resolveFunction(analysis_bundle.analysis, *options->function_selector);
      if (function == nullptr)
      {
        std::cerr << "function not found: " << *options->function_selector << '\n';
        return EXIT_FAILURE;
      }

      if (options->dot_path.has_value())
      {
        const Status status =
            writeTextFile(*options->dot_path, DotExporter::exportFunctionCfg(*function));
        if (!status)
        {
          std::cerr << status.error().message << '\n';
          return EXIT_FAILURE;
        }
      }

      return emit(
          *options,
          [&]() {
            ProgramAnalysis single;
            single.functions = {*function};
            return JsonWriter::renderFunctions(single);
          },
          TerminalRenderer::renderCfg(*function));
    }

    if (options->command == "callgraph")
    {
      if (options->dot_path.has_value())
      {
        const Status status =
            writeTextFile(*options->dot_path, DotExporter::exportCallGraph(analysis_bundle.analysis));
        if (!status)
        {
          std::cerr << status.error().message << '\n';
          return EXIT_FAILURE;
        }
      }

      return emit(
          *options,
          [&]() { return JsonWriter::renderFunctions(analysis_bundle.analysis); },
          TerminalRenderer::renderCallGraph(analysis_bundle.analysis));
    }

    if (options->command == "heuristics")
    {
      return emit(
          *options,
          [&]() { return JsonWriter::renderHeuristics(analysis_bundle.heuristic_findings); },
          TerminalRenderer::renderHeuristics(analysis_bundle.heuristic_findings, options->verbose));
    }

    if (options->command == "analyze")
    {
      if (options->full || options->dot_directory.has_value())
      {
        const std::filesystem::path dot_dir =
            options->dot_directory.value_or(std::filesystem::path("graphs"));
        if (const Status status = ensureDirectory(dot_dir); !status)
        {
          std::cerr << status.error().message << '\n';
          return EXIT_FAILURE;
        }

        if (const Status status =
                writeTextFile(dot_dir / "callgraph.dot", DotExporter::exportCallGraph(analysis_bundle.analysis));
            !status)
        {
          std::cerr << status.error().message << '\n';
          return EXIT_FAILURE;
        }

        for (const RecoveredFunction& function : analysis_bundle.analysis.functions)
        {
          const std::string file_name =
              sanitizeName(function.name) + "_" + formatHex(function.entry).substr(2) + ".dot";
          if (const Status status =
                  writeTextFile(dot_dir / file_name, DotExporter::exportFunctionCfg(function));
              !status)
          {
            std::cerr << status.error().message << '\n';
            return EXIT_FAILURE;
          }
        }
      }

      return emit(
          *options,
          [&]() { return JsonWriter::renderAnalysis(analysis_bundle); },
          TerminalRenderer::renderAnalysis(analysis_bundle, options->verbose));
    }
  }

  printUsage();
  return EXIT_FAILURE;
}
