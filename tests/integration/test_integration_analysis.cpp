#include <catch2/catch_test_macros.hpp>

#include "test_support.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("Direct call fixture recovers functions and call graph edges")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("direct_calls");
  REQUIRE(bundle);

  const RecoveredFunction* caller =
      test_support::findFunctionByName(bundle.value().analysis, "caller");
  const RecoveredFunction* callee =
      test_support::findFunctionByName(bundle.value().analysis, "callee");
  REQUIRE(caller != nullptr);
  REQUIRE(callee != nullptr);
  CHECK(test_support::hasCallEdge(bundle.value().analysis, caller->entry, callee->entry));
}

TEST_CASE("Loop fixture exposes a looping function with nontrivial complexity")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("loop");
  REQUIRE(bundle);

  const RecoveredFunction* function =
      test_support::findFunctionByName(bundle.value().analysis, "accumulate");
  REQUIRE(function != nullptr);
  CHECK(function->metrics.loop_count >= 1);
  CHECK(function->metrics.cyclomatic_complexity >= 2);
}

TEST_CASE("Switch fixture suggests a jump-table-like dispatch")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("switch_case");
  REQUIRE(bundle);

  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_jump_table"));
}

TEST_CASE("Virtual dispatch fixture surfaces vtable and indirect dispatch findings")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("virtual_dispatch");
  REQUIRE(bundle);

  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_virtual_dispatch"));
  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_vtable_region"));
}

TEST_CASE("Graphics fixture surfaces render and runtime heuristic findings")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("graphics_keywords");
  REQUIRE(bundle);

  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_render_api_hook"));
  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_vr_runtime_hook"));
  CHECK(test_support::hasFinding(bundle.value().heuristic_findings, "candidate_engine_fingerprint"));
}

TEST_CASE("Stripped fixtures still produce partial structural recovery")
{
  const Result<AnalysisBundle> bundle = test_support::analyzeFixture("direct_calls_stripped");
  REQUIRE(bundle);

  CHECK_FALSE(bundle.value().analysis.functions.empty());
  CHECK(bundle.value().analysis.metrics.function_count >= 1);
}

}  // namespace
