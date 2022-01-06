/*
 * Copyright (C) 2022 Armands Aleksejevs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

#pragma once

/*
 * includes
 */
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief The MainWindow class
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow( QWidget *parent = nullptr );
    ~MainWindow() override;

protected:
    bool nativeEvent( const QByteArray &eventType, void *message, qintptr *result ) override;
    bool eventFilter( QObject *watched, QEvent *event ) override;

private slots:
    void updateShortcut();
    void registerShortcut( const QKeySequence &sequence );
    void makeTrayIcon();

private:
    Ui::MainWindow *ui;
    QTimer timer;
    QKeySequence sequence;
    QKeySequence capturedSequence;
    QSystemTrayIcon *tray;
};
