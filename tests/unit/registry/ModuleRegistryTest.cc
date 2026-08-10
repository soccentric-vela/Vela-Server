// tests/unit/registry/ModuleRegistryTest.cc
#include "registry/ModuleRegistry.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdlib>

using vela::ModuleRegistry;

// Write a temp YAML file and verify load().
TEST(ModuleRegistryTest, LoadFromYaml) {
    auto tmp = std::filesystem::temp_directory_path() / "vela_test_registry.yaml";
    {
        std::ofstream f{tmp};
        f << R"(
modules:
  - id: "mod.temp"
    name: "Temperature"
    description: "Reads temperature sensor"
    version: "1.0.0"
    result_type_url: "type.googleapis.com/vela.TempResult"
    config_type_url: "type.googleapis.com/vela.TempConfig"
    modes: ["single_shot", "monitoring"]
)";
    }

    ModuleRegistry reg;
    reg.load(tmp.string());
    EXPECT_EQ(reg.size(), 1u);
    EXPECT_TRUE(reg.has("mod.temp"));

    auto m = reg.find("mod.temp");
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->name, "Temperature");

    std::filesystem::remove(tmp);
}

TEST(ModuleRegistryTest, MissingIdWarnsAndSkips) {
    auto tmp = std::filesystem::temp_directory_path() / "vela_test_registry_bad.yaml";
    {
        std::ofstream f{tmp};
        f << R"(
modules:
  - id: ""
    name: "NoId"
    version: "0.0.1"
    modes: []
)";
    }
    ModuleRegistry reg;
    reg.load(tmp.string());
    EXPECT_EQ(reg.size(), 0u);
    std::filesystem::remove(tmp);
}
