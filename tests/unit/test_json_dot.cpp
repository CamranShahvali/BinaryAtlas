#include <filesystem>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "binaryatlas/graph/dot_exporter.hpp"
#include "binaryatlas/output/json_writer.hpp"
#include "binaryatlas/util/file_io.hpp"
#include "test_support.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("JSON writer emits expected top-level analysis schema")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("direct_calls");
  REQUIRE(bundle);

  const nlohmann::json json = nlohmann::json::parse(JsonWriter::renderAnalysis(bundle.value()));
  CHECK(json.contains("metadata"));
  CHECK(json.contains("sections"));
  CHECK(json.contains("disassembly"));
  CHECK(json.contains("analysis"));
  CHECK(json.contains("heuristics"));
  CHECK(json["analysis"].contains("functions"));
  CHECK(json["analysis"].contains("call_graph"));
}

TEST_CASE("DOT exporter emits non-empty CFG and call graph text")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("direct_calls");
  REQUIRE(bundle);

  const RecoveredFunction* caller =
      test_support::findFunctionByName(bundle.value().analysis, "caller");
  REQUIRE(caller != nullptr);

  const std::string cfg = DotExporter::exportFunctionCfg(*caller);
  const std::string callgraph = DotExporter::exportCallGraph(bundle.value().analysis);
  CHECK(cfg.starts_with("digraph"));
  CHECK(callgraph.starts_with("digraph"));

  const std::filesystem::path dot_path =
      std::filesystem::temp_directory_path() / "binaryatlas_callgraph_test.dot";
  const Status status = writeTextFile(dot_path, callgraph);
  REQUIRE(status);
  CHECK(std::filesystem::exists(dot_path));
  CHECK(std::filesystem::file_size(dot_path) > 0);
}

} // namespace
