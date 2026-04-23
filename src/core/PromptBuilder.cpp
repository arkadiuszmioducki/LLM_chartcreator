#include "PromptBuilder.h"
#include <QMap>

namespace core {

QString PromptBuilder::buildSystemPrompt(TaskType task) const {
    switch (task) {
    case TaskType::ChartGeneration:
        return QString(
            "You are a Python code generator specialized in matplotlib charts.\n"
            "STRICT RULES - follow every rule exactly:\n"
            "1. Output ONLY raw Python code. No markdown fences, no explanations, no comments outside code.\n"
            "2. Import: matplotlib.pyplot as plt, numpy as np. Import pandas only if needed.\n"
            "3. NEVER use plt.show(). ALWAYS use plt.savefig().\n"
            "4. NEVER use plt.cm.get_cmap() - it is deprecated. "
            "Use matplotlib.colormaps[name] or simple color lists instead.\n"
            "5. For bar/pie colors use a plain Python list like: "
            "colors = ['#1f77b4','#ff7f0e','#2ca02c','#d62728','#9467bd','#8c564b']\n"
            "6. For colormap: import matplotlib.colormaps; cmap = matplotlib.colormaps['viridis']\n"
            "7. Wrap everything in try/except Exception as e: print(f'Wystapil blad: {e}')\n"
            "8. Last line must be exactly: print('WYKRES_ZAPISANY: <output_path>')\n"
            "9. Response = only Python code, starting with 'import' or 'try:'. Nothing else."
        );

    case TaskType::NetworkDiagnosis:
        return QString(
            "You are a Python code generator for network diagnostics.\n"
            "RULES:\n"
            "1. Output ONLY raw Python code. No markdown, no explanations.\n"
            "2. Use only stdlib: subprocess, platform, socket, datetime, sys.\n"
            "3. Detect OS automatically (platform.system()) and use correct ping syntax.\n"
            "   Windows: ping -n <count> -w <timeout_ms> <host>\n"
            "   Linux/Mac: ping -c <count> -W <timeout_s> <host>\n"
            "4. Print results as a formatted table.\n"
            "5. Print summary: reachable count, unreachable count.\n"
            "6. Handle all exceptions.\n"
            "7. Last line: print('DIAGNOSTYKA_ZAKONCZONA')\n"
            "8. Response = only Python code."
        );

    case TaskType::SystemCommand:
        return QString(
            "You are a Python generator for safe system commands.\n"
            "RULES:\n"
            "1. Output ONLY raw Python code.\n"
            "2. Use subprocess module. Detect OS via platform.system().\n"
            "3. Never perform destructive operations.\n"
            "4. Handle exceptions.\n"
            "5. Response = only Python code."
        );

    case TaskType::DataAnalysis:
        return QString(
            "You are a Python data analysis code generator.\n"
            "RULES:\n"
            "1. Output ONLY raw Python code.\n"
            "2. Use pandas, numpy. Handle missing values.\n"
            "3. Print descriptive statistics.\n"
            "4. Response = only Python code."
        );

    default:
        return "You are a Python code generator. Output only valid Python code, no explanations.";
    }
}

QString PromptBuilder::buildUserPrompt(const ChartParameters& params) const {
    QString outPath = params.outputPath.isEmpty() ? "wykres.png" : params.outputPath;
    QString outFmt  = params.outputFormat.isEmpty() ? "png" : params.outputFormat;

    QString prompt;
    prompt += QString("Generate Python code that creates a %1 chart.\n\n")
                  .arg(chartTypeToString(params.type));

    prompt += "CHART PARAMETERS:\n";
    if (!params.title.isEmpty())
        prompt += QString("- Title: %1\n").arg(params.title);
    if (!params.xLabel.isEmpty())
        prompt += QString("- X axis label: %1\n").arg(params.xLabel);
    if (!params.yLabel.isEmpty())
        prompt += QString("- Y axis label: %1\n").arg(params.yLabel);

    // Map color scheme to safe matplotlib names
    QString safeColor = mapColorScheme(params.colorScheme);
    prompt += QString("- Colors: use the list %1\n").arg(safeColor);

    prompt += QString("- Show grid: %1\n").arg(params.showGrid ? "yes" : "no");
    prompt += QString("- Show legend: %1\n").arg(params.showLegend ? "yes" : "no");
    prompt += QString("- Figure size: %1x%2 inches\n").arg(params.figWidth).arg(params.figHeight);
    prompt += QString("- Output file: %1 (format: %2, dpi=150)\n").arg(outPath).arg(outFmt);

    if (!params.dataInput.isEmpty()) {
        prompt += "\nINPUT DATA:\n";
        prompt += formatDataForPrompt(params.dataInput);
        prompt += "\n";
    }

    // Additional instructions: placed BEFORE the mandatory structure so the model sees them first
    if (!params.additionalInfo.isEmpty()) {
        prompt += "\n=== PRIORITY ADDITIONAL REQUIREMENTS (MUST implement these) ===\n";
        prompt += params.additionalInfo + "\n";
        prompt += "=== END OF ADDITIONAL REQUIREMENTS ===\n";
    }

    prompt += QString(
        "\nMANDATORY CODE STRUCTURE:\n"
        "import matplotlib\n"
        "import matplotlib.pyplot as plt\n"
        "import numpy as np\n"
        "try:\n"
        "    fig, ax = plt.subplots(figsize=(%1, %2))\n"
        "    # ... chart code here ...\n"
        "    plt.savefig('%3', dpi=150, bbox_inches='tight')\n"
        "    plt.close()\n"
        "    print('WYKRES_ZAPISANY: %3')\n"
        "except Exception as e:\n"
        "    print(f'Wystapil blad: {e}')\n"
        "Fill in the chart code. Output only complete Python code."
    ).arg(params.figWidth).arg(params.figHeight).arg(outPath);

    return prompt;
}

QString PromptBuilder::mapColorScheme(const QString& scheme) const {
    // Return safe explicit color lists instead of colormap names
    // This avoids deprecated get_cmap() and array-as-color bugs
    static const QMap<QString, QString> colorMap = {
        {"tab10",    "['#1f77b4','#ff7f0e','#2ca02c','#d62728','#9467bd','#8c564b','#e377c2','#7f7f7f','#bcbd22','#17becf']"},
        {"Set2",     "['#66c2a5','#fc8d62','#8da0cb','#e78ac3','#a6d854','#ffd92f','#e5c494','#b3b3b3']"},
        {"Set3",     "['#8dd3c7','#ffffb3','#bebada','#fb8072','#80b1d3','#fdb462','#b3de69','#fccde5']"},
        {"Blues",    "['#084594','#2171b5','#4292c6','#6baed6','#9ecae1','#c6dbef','#deebf7','#f7fbff']"},
        {"Reds",     "['#99000d','#cb181d','#ef3b2c','#fb6a4a','#fc9272','#fcbba1','#fee0d2','#fff5f0']"},
        {"Greens",   "['#005a32','#238b45','#41ab5d','#74c476','#a1d99b','#c7e9c0','#e5f5e0','#f7fcf5']"},
        {"viridis",  "['#440154','#31688e','#35b779','#fde725']"},
        {"plasma",   "['#0d0887','#7e03a8','#cc4778','#f89441','#f0f921']"},
        {"niebiesko-pomaranzowy", "['#1f77b4','#ff7f0e','#1f77b4','#ff7f0e','#aec7e8','#ffbb78']"},
    };
    return colorMap.value(scheme,
        "['#1f77b4','#ff7f0e','#2ca02c','#d62728','#9467bd','#8c564b']");
}

QString PromptBuilder::buildNetworkPrompt(const QStringList& hosts, int pingCount, int timeout) const {
    QString prompt = "Generate Python code for network diagnostics.\n\n";
    prompt += "HOSTS TO CHECK:\n";
    for (const auto& h : hosts)
        prompt += QString("- %1\n").arg(h);
    prompt += QString("\nPING COUNT: %1\n").arg(pingCount);
    prompt += QString("TIMEOUT: %1 seconds\n\n").arg(timeout);
    prompt += "For each host: run ping, measure response time, check availability.\n";
    prompt += "Display a formatted table with results and a summary.\n";
    prompt += "Detect OS automatically. Handle all exceptions.\n";
    prompt += "Last line: print('DIAGNOSTYKA_ZAKONCZONA')";
    return prompt;
}

QString PromptBuilder::buildSystemCommandPrompt(const QString& description, const QString& osType) const {
    return QString("Generate Python code that: %1\n"
                   "Target OS: %2\n"
                   "Use subprocess. Print the result.")
        .arg(description).arg(osType);
}

QString PromptBuilder::chartTypeToString(ChartType type) {
    static const QMap<ChartType, QString> map = {
        {ChartType::Line,      "line"},
        {ChartType::Bar,       "bar"},
        {ChartType::Scatter,   "scatter"},
        {ChartType::Histogram, "histogram"},
        {ChartType::Pie,       "pie"},
    };
    return map.value(type, "line");
}

ChartType PromptBuilder::stringToChartType(const QString& str) {
    static const QMap<QString, ChartType> map = {
        {"Line",      ChartType::Line},
        {"Bar",       ChartType::Bar},
        {"Scatter",   ChartType::Scatter},
        {"Histogram", ChartType::Histogram},
        {"Pie",       ChartType::Pie},
    };
    return map.value(str, ChartType::Line);
}

QString PromptBuilder::formatDataForPrompt(const QString& data) const {
    QString trimmed = data.trimmed();
    if (trimmed.isEmpty()) return "(no data provided)";
    if (trimmed.length() > 2000)
        return trimmed.left(2000) + "\n...(data truncated)";
    return trimmed;
}

} // namespace core
