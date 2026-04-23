/**
 * @file test_PromptBuilder.cpp
 * @brief Testy jednostkowe klasy PromptBuilder.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

#include <gtest/gtest.h>
#include "../src/core/PromptBuilder.h"

using namespace core;

class PromptBuilderTest : public ::testing::Test {
protected:
    PromptBuilder builder;
};

// ===== buildSystemPrompt =====

TEST_F(PromptBuilderTest, SystemPromptChartGenerationNotEmpty) {
    QString prompt = builder.buildSystemPrompt(TaskType::ChartGeneration);
    EXPECT_FALSE(prompt.isEmpty());
}

TEST_F(PromptBuilderTest, SystemPromptChartContainsPython) {
    QString prompt = builder.buildSystemPrompt(TaskType::ChartGeneration);
    EXPECT_TRUE(prompt.contains("Python", Qt::CaseInsensitive));
}

TEST_F(PromptBuilderTest, SystemPromptChartForbidsGetCmap) {
    QString prompt = builder.buildSystemPrompt(TaskType::ChartGeneration);
    // Must explicitly warn about deprecated get_cmap
    EXPECT_TRUE(prompt.contains("get_cmap", Qt::CaseInsensitive) ||
                prompt.contains("deprecated", Qt::CaseInsensitive) ||
                prompt.contains("colormaps", Qt::CaseInsensitive));
}

TEST_F(PromptBuilderTest, SystemPromptChartForbidsPlotShow) {
    QString prompt = builder.buildSystemPrompt(TaskType::ChartGeneration);
    EXPECT_TRUE(prompt.contains("plt.show") || prompt.contains("savefig"));
}

TEST_F(PromptBuilderTest, SystemPromptNetworkNotEmpty) {
    QString prompt = builder.buildSystemPrompt(TaskType::NetworkDiagnosis);
    EXPECT_FALSE(prompt.isEmpty());
}

TEST_F(PromptBuilderTest, SystemPromptAllTypesReturnNonEmpty) {
    EXPECT_FALSE(builder.buildSystemPrompt(TaskType::ChartGeneration).isEmpty());
    EXPECT_FALSE(builder.buildSystemPrompt(TaskType::NetworkDiagnosis).isEmpty());
    EXPECT_FALSE(builder.buildSystemPrompt(TaskType::SystemCommand).isEmpty());
    EXPECT_FALSE(builder.buildSystemPrompt(TaskType::DataAnalysis).isEmpty());
}

// ===== buildUserPrompt =====

TEST_F(PromptBuilderTest, UserPromptIncludesChartType) {
    ChartParameters p;
    p.type = ChartType::Bar;
    p.dataInput = "1,2,3\n10,20,30";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("bar", Qt::CaseInsensitive));
}

TEST_F(PromptBuilderTest, UserPromptIncludesTitle) {
    ChartParameters p;
    p.title = "Test Chart ABC";
    p.dataInput = "1,2,3";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("Test Chart ABC"));
}

TEST_F(PromptBuilderTest, UserPromptIncludesAxisLabels) {
    ChartParameters p;
    p.xLabel = "Time [s]";
    p.yLabel = "Amplitude [V]";
    p.dataInput = "1,2,3";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("Time [s]"));
    EXPECT_TRUE(prompt.contains("Amplitude [V]"));
}

TEST_F(PromptBuilderTest, UserPromptIncludesOutputPath) {
    ChartParameters p;
    p.outputPath = "output/my_chart.png";
    p.dataInput = "1,2";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("my_chart.png"));
}

TEST_F(PromptBuilderTest, UserPromptColorSchemeContainsHexColors) {
    ChartParameters p;
    p.colorScheme = "tab10";
    p.dataInput = "1,2,3";
    QString prompt = builder.buildUserPrompt(p);
    // Should contain hex color codes, not a colormap array
    EXPECT_TRUE(prompt.contains("#"));
}

TEST_F(PromptBuilderTest, UserPromptHandlesEmptyTitle) {
    ChartParameters p;
    p.dataInput = "1,2,3\n10,20,30";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_FALSE(prompt.isEmpty());
}

TEST_F(PromptBuilderTest, UserPromptDataLimitedTo2000Chars) {
    ChartParameters p;
    p.dataInput = QString(3000, 'x');
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("truncated") || prompt.contains("skroc") || prompt.length() < 6000);
}

TEST_F(PromptBuilderTest, UserPromptIncludesFigSize) {
    ChartParameters p;
    p.figWidth = 12;
    p.figHeight = 8;
    p.dataInput = "1,2";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("12") && prompt.contains("8"));
}

TEST_F(PromptBuilderTest, UserPromptContainsSavefig) {
    ChartParameters p;
    p.outputPath = "result.png";
    p.dataInput = "1,2";
    QString prompt = builder.buildUserPrompt(p);
    EXPECT_TRUE(prompt.contains("savefig"));
}

// ===== buildNetworkPrompt =====

TEST_F(PromptBuilderTest, NetworkPromptIncludesAllHosts) {
    QStringList hosts = {"8.8.8.8", "1.1.1.1", "google.com"};
    QString prompt = builder.buildNetworkPrompt(hosts, 4, 5);
    for (const auto& h : hosts) {
        EXPECT_TRUE(prompt.contains(h)) << h.toStdString();
    }
}

TEST_F(PromptBuilderTest, NetworkPromptIncludesPingCount) {
    QString prompt = builder.buildNetworkPrompt({"8.8.8.8"}, 7, 3);
    EXPECT_TRUE(prompt.contains("7"));
}

TEST_F(PromptBuilderTest, NetworkPromptNotEmpty) {
    QString prompt = builder.buildNetworkPrompt({"localhost"}, 1, 1);
    EXPECT_FALSE(prompt.isEmpty());
}

// ===== chartTypeToString / stringToChartType =====

TEST_F(PromptBuilderTest, ChartTypeToStringNotEmpty) {
    EXPECT_FALSE(PromptBuilder::chartTypeToString(ChartType::Line).isEmpty());
    EXPECT_FALSE(PromptBuilder::chartTypeToString(ChartType::Bar).isEmpty());
    EXPECT_FALSE(PromptBuilder::chartTypeToString(ChartType::Scatter).isEmpty());
    EXPECT_FALSE(PromptBuilder::chartTypeToString(ChartType::Histogram).isEmpty());
    EXPECT_FALSE(PromptBuilder::chartTypeToString(ChartType::Pie).isEmpty());
}

TEST_F(PromptBuilderTest, ChartTypeRoundTrip) {
    QStringList types = {"Line", "Bar", "Scatter", "Histogram", "Pie"};
    for (const auto& t : types) {
        ChartType ct = PromptBuilder::stringToChartType(t);
        EXPECT_FALSE(PromptBuilder::chartTypeToString(ct).isEmpty());
    }
}

TEST_F(PromptBuilderTest, UnknownTypeDefaultsToLine) {
    ChartType ct = PromptBuilder::stringToChartType("NonExistent");
    EXPECT_EQ(ct, ChartType::Line);
}
