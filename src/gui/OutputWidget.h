#pragma once
#include <QWidget>

/**
 * @file OutputWidget.h
 * @brief Widget do wyświetlania wyników wykonania kodu.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace gui {
/** @brief Placeholder - funkcjonalność wbudowana w MainWindow. */
class OutputWidget : public QWidget {
    Q_OBJECT
public:
    explicit OutputWidget(QWidget* parent = nullptr) : QWidget(parent) {}
};
} // namespace gui
