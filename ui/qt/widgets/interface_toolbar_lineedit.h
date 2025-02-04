/* interface_toolbar_lineedit.h
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef INTERFACE_TOOLBAR_LINEEDIT_H
#define INTERFACE_TOOLBAR_LINEEDIT_H

#include <QLineEdit>
#include <QRegExp>

class StockIconToolButton;

class InterfaceToolbarLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit InterfaceToolbarLineEdit(QWidget *parent = 0, QString validation_regex = QString(), bool is_required = false);
    void disableApplyButton();

protected:
    void resizeEvent(QResizeEvent *);

signals:
    void editedTextApplied();

private slots:
    void validateText();
    void validateEditedText();
    void applyEditedText();

private:
    bool isValid();
    void updateStyleSheet(bool is_valid);

    StockIconToolButton *apply_button_;
    QRegExp regex_expr_;
    bool is_required_;
    bool text_edited_;
};

#endif // INTERFACE_TOOLBAR_LINEEDIT_H
