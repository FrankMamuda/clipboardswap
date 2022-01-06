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

/*
 * includes
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QSettings>
#include <QStyle>
#include <QMenu>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
MainWindow::MainWindow( QWidget *parent ): QMainWindow( parent ), ui( new Ui::MainWindow ) {
    this->ui->setupUi( this );

    QSettings settings;
    this->sequence = QKeySequence::fromString( settings.value( "sequence", "Ctrl+b" ).toString());
    this->registerShortcut( this->sequence );

    this->ui->shortcutEdit->setReadOnly( true );
    this->ui->shortcutEdit->setText( sequence.toString());

    this->timer.setSingleShot( true );
    this->timer.setInterval( 3000 );

    QPushButton::connect( this->ui->shortcutButton, &QPushButton::clicked, [ this ]() {
        this->timer.start();
        this->ui->shortcutEdit->setEnabled( false );
        this->ui->shortcutEdit->setText( MainWindow::tr( "Waiting for key sequence..." ));
        this->ui->shortcutButton->setEnabled( false );
    } );

    QTimer::connect( &this->timer, &QTimer::timeout, [ this ]() {
        this->updateShortcut();
    } );

    this->installEventFilter( this );

#ifdef Q_OS_WIN
    this->setWindowIcon( this->style()->standardIcon( QStyle::SP_MessageBoxInformation ));
#endif

    this->setWindowFlags( this->windowFlags() | Qt::Dialog | Qt::Tool );
    this->makeTrayIcon();

#if 1
    this->ui->startCheck->hide();
#endif
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow() {
    delete this->ui;
}

/**
 * @brief MainWindow::nativeEvent
 * @param eventType
 * @param message
 * @param result
 * @return
 */
bool MainWindow::nativeEvent( const QByteArray &eventType, void *message, qintptr *result ) {
#ifdef Q_OS_WIN
    MSG *msg = reinterpret_cast<MSG*>( message );
    if ( msg->message == WM_HOTKEY && !this->timer.isActive()) {
        auto getClipboardText = []() {
            QString text;

            if ( !OpenClipboard( nullptr ))
                return text;

            const HANDLE dataHandle = GetClipboardData( CF_UNICODETEXT );
            if ( dataHandle == nullptr )
                return text;

            const wchar_t *clipBoardText = static_cast<wchar_t*>( GlobalLock( dataHandle ));
            if ( clipBoardText == nullptr )
                return text;

            GlobalUnlock( dataHandle );
            CloseClipboard();

            return QString::fromWCharArray( clipBoardText );
        };

        auto setClipboardText = []( const QString &text ) {
            if ( !OpenClipboard( nullptr ))
                return false;

            EmptyClipboard();
            const size_t size = static_cast<size_t>(( text.length() + 1 ) * 2 );

            const HGLOBAL globalHandle = GlobalAlloc( GMEM_ZEROINIT, size );
            if ( globalHandle == nullptr )
                return false;

            wchar_t *data = static_cast<LPTSTR>( GlobalLock( globalHandle ));
            if ( data == nullptr )
                return false;

            const std::wstring textWString = text.toStdWString();
            const wchar_t *textWArray = const_cast<wchar_t *>( textWString.c_str());
            memcpy( data, textWArray, size );

            GlobalUnlock( globalHandle );
            SetClipboardData( CF_UNICODETEXT, globalHandle );

            CloseClipboard();

            return true;
        };

        auto swapClipboard = [ getClipboardText, setClipboardText ]() {
            const QString oldText( getClipboardText());
            if ( oldText.isEmpty())
                return;

            //
            // COPY SELECTION TO CLIPBOARD
            //
            INPUT sequence[4];
            bool controlDown = GetKeyState( VK_CONTROL ) < 0;

            // CTRL down
            if ( !controlDown ) {
                sequence[0].ki.wVk = VK_CONTROL;
                sequence[0].ki.dwFlags = 0;
                sequence[0].type = INPUT_KEYBOARD;
            }

            // C down
            sequence[1].ki.wVk = 'C';
            sequence[1].ki.dwFlags = 0;
            sequence[1].type = INPUT_KEYBOARD;

            // C up
            sequence[2].ki.wVk = 'C';
            sequence[2].ki.dwFlags = KEYEVENTF_KEYUP;
            sequence[2].type = INPUT_KEYBOARD;

            // CTRL up
            if ( !controlDown ) {
                sequence[3].ki.wVk = VK_CONTROL;
                sequence[3].ki.dwFlags = KEYEVENTF_KEYUP;
                sequence[3].type = INPUT_KEYBOARD;
            }

            SendInput( 4, sequence, sizeof( INPUT ));

            // wait
            Sleep( 20 );

            // get copied text from clipboard
            const QString newText( getClipboardText());
            if ( newText.isEmpty())
                return;

            //
            // SET INITIAL TEXT TO CLIPBOARD
            //
            if ( !setClipboardText( oldText ))
                return;

            //
            // PASTE INITIAL TEXT
            //

            // CTRL down
            controlDown = GetKeyState( VK_CONTROL ) < 0;

            if ( !controlDown ) {
                sequence[0].ki.wVk = VK_CONTROL;
                sequence[0].ki.dwFlags = 0;
                sequence[0].type = INPUT_KEYBOARD;
            }

            // C down
            sequence[1].ki.wVk = 'V';
            sequence[1].ki.dwFlags = 0;
            sequence[1].type = INPUT_KEYBOARD;

            // C up
            sequence[2].ki.wVk = 'V';
            sequence[2].ki.dwFlags = KEYEVENTF_KEYUP;
            sequence[2].type = INPUT_KEYBOARD;

            // CTRL up
            if ( !controlDown ) {
                sequence[3].ki.wVk = VK_CONTROL;
                sequence[3].ki.dwFlags = KEYEVENTF_KEYUP;
                sequence[3].type = INPUT_KEYBOARD;
            }

            SendInput( 4, sequence, sizeof( INPUT ));

            // wait
            Sleep( 20 );

            //
            // SET NEW TEXT TO CLIPBOARD
            //
            setClipboardText( newText );
        };

        swapClipboard();
        return true;
    }
#endif
    return QMainWindow::nativeEvent( eventType, message, result );
}

/**
 * @brief MainWindow::eventFilter
 * @param watched
 * @param event
 * @return
 */
bool MainWindow::eventFilter( QObject *watched, QEvent *event ) {
    if ( this->timer.isActive()) {
        if ( event->type() == QEvent::KeyRelease ) {
            QKeyEvent *keyEvent( dynamic_cast<QKeyEvent *>( event ));

            const Qt::Key key = static_cast<Qt::Key>( keyEvent->key());
            Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

            if ( key != Qt::Key_Control && key != Qt::Key_Shift && key != Qt::Key_Alt && key != Qt::Key_Meta && modifiers != Qt::NoModifier ) {
                int mod = 0;

                if ( modifiers & Qt::ShiftModifier )
                    mod += Qt::SHIFT;
                if ( modifiers & Qt::ControlModifier )
                    mod += Qt::CTRL;
                if ( modifiers & Qt::AltModifier )
                    mod += Qt::ALT;

                if ( mod != 0 ) {
                    qDebug() << "caught event" << keyEvent->modifiers() << QKeySequence( static_cast<int>( key ) | mod ).toString();
                    this->capturedSequence = QKeySequence( static_cast<int>( key ) | mod );

                    this->timer.stop();
                    this->updateShortcut();

                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter( watched, event );
}

/**
 * @brief MainWindow::updateShortcut
 */
void MainWindow::updateShortcut() {
    const QKeySequence sequence( this->capturedSequence.isEmpty() ? this->sequence : this->capturedSequence );

    this->ui->shortcutButton->setEnabled( true );
    this->ui->shortcutEdit->setEnabled( true );
    this->ui->shortcutEdit->setText( sequence.toString());

    this->registerShortcut( sequence );
}

/**
 * @brief MainWindow::registerShortcut
 * @param sequence
 */
void MainWindow::registerShortcut( const QKeySequence &sequence ) {
#ifdef Q_OS_WIN
    UnregisterHotKey( reinterpret_cast<HWND>( this->window()->winId()), 100 );

    QStringList sequenceList( sequence.toString().split( "+" ));
    unsigned int mod = 0;
    qsizetype index;

    index = sequenceList.indexOf( "Ctrl", Qt::CaseInsensitive );
    if ( index != -1 ) {
        mod += MOD_CONTROL;
        sequenceList.takeAt( index );
    }

    index = sequenceList.indexOf( "Shift", Qt::CaseInsensitive );
    if ( index != -1 ) {
        mod += MOD_SHIFT;
        sequenceList.takeAt( index );
    }

    index = sequenceList.indexOf( "Alt", Qt::CaseInsensitive );
    if ( index != -1 ) {
        mod += MOD_ALT;
        sequenceList.takeAt( index );
    }

    if ( mod != 0 && sequenceList.count()) {
        if ( RegisterHotKey( reinterpret_cast<HWND>( this->window()->winId()), 100, mod, static_cast<unsigned int>( sequenceList.first().at( 0 ).unicode()))) {
            qDebug() << "hotkey" << sequence.toString() << "registered";
        }
    }
#endif
}

/**
 * @brief MainWindow::makeTrayIcon
 */
void MainWindow::makeTrayIcon() {
    this->tray = new QSystemTrayIcon( this->windowIcon(), this );
   // this->hide();

    auto toggleState = [ this ]( QSystemTrayIcon::ActivationReason activationReason = QSystemTrayIcon::Unknown ) {
        if ( activationReason == QSystemTrayIcon::DoubleClick ) {
            if ( !this->isVisible()) {
                this->show();
            } else {
                this->hide();
            }
        }
    };

    QSystemTrayIcon::connect( this->tray, &QSystemTrayIcon::activated, toggleState );

    QMenu *menu( new QMenu());
    menu->addAction( this->style()->standardIcon( QStyle::SP_TitleBarNormalButton ), MainWindow::tr( "Show/Hide" ), [ toggleState ]() { toggleState( QSystemTrayIcon::DoubleClick ); } );
    menu->addSeparator();
    menu->addAction( this->style()->standardIcon( QStyle::SP_TitleBarCloseButton ), MainWindow::tr( "Quit" ), [](){ qApp->quit(); } );

    this->tray->setContextMenu( menu );
    this->tray->show();
}
