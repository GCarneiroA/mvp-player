#include "MVPPlayerLocalDialog.hpp"
#include "MVPPlayerSettingsDialog.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDesktopWidget>

#include <iostream>

namespace mvpplayer
{
namespace gui
{
namespace qt
{

MVPPlayerLocalDialog::MVPPlayerLocalDialog( QWidget *parent )
: Parent( parent )
{
    widget.setupUi(this);
    
    initDialog( widget );
    connect( _btnNext, SIGNAL( clicked(bool) ), this, SLOT( slotViewHitNextBtn() ) );
    connect( _btnPrevious, SIGNAL( clicked(bool) ), this, SLOT( slotViewHitPreviousBtn() ) );
    connect( _btnPlayPause, SIGNAL( clicked(bool) ), this, SLOT( slotViewHitPlayStopBtn() ) );
    connect( widget.btnServer, SIGNAL( clicked(bool) ), this, SLOT( startStopServer( const bool ) ) );
    connect( widget.playlist, SIGNAL( currentRowChanged(int) ), this, SLOT( playPlaylistItemAtIndex(int) ) );
    connect( widget.btnSettings, SIGNAL( clicked(bool) ), this, SLOT( editSettings() ) );
}

MVPPlayerLocalDialog::~MVPPlayerLocalDialog()
{
}

boost::filesystem::path MVPPlayerLocalDialog::openFile( const std::string & title, const std::string & extensions )
{
    QString result;
    QMetaObject::invokeMethod( this, "slotOpenFile", Qt::BlockingQueuedConnection, Q_RETURN_ARG( QString, result ), Q_ARG( QString, QString::fromStdString( title ) ), Q_ARG( QString, QString::fromStdString( extensions ) ) );
    return result.toStdString();
}

void MVPPlayerLocalDialog::slotViewHitPlayStopBtn()
{
    if ( _btnPlayPause->isChecked() == false )
    {
        signalViewHitStop();
    }
    else
    {
        signalViewHitPlay( boost::none );
    }
}

void MVPPlayerLocalDialog::slotViewHitPreviousBtn()
{
    signalViewHitPrevious();
}

void MVPPlayerLocalDialog::slotViewHitNextBtn()
{
    signalViewHitNext();
}

void MVPPlayerLocalDialog::playPlaylistItemAtIndex( const int playlistIndex )
{
    signalViewHitPlaylistItem( playlistIndex );
}

void MVPPlayerLocalDialog::openedPlaylist( const std::vector<m3uParser::PlaylistItem> & playlistItems )
{
    QStringList filenames;
    for( const m3uParser::PlaylistItem & item: playlistItems )
    {
        filenames.append( item.filename.string().c_str() );
    }
    QMetaObject::invokeMethod( this, "slotOpenedPlaylist", Qt::BlockingQueuedConnection, Q_ARG( QStringList, filenames ) );
}

void MVPPlayerLocalDialog::dropEvent( QDropEvent *de )
{
    // Unpack dropped data and handle it
    if ( de->mimeData()->hasUrls() )
    {
        QList<QUrl> urlList = de->mimeData()->urls();
        if ( urlList.size() && urlList.first().path().endsWith( ".m3u" ) )
        {
            signalSequencial(
                [this, &urlList]()
                {
                    signalViewClearPlaylist();
                    signalViewHitPlay( boost::filesystem::path( urlList.first().path().toStdString() ) );
                }
            );
        }
        else
        {
            signalSequencial(
                [this, &urlList](){
                    signalViewHitStop();
                    signalViewClearPlaylist();
                    for ( int i = 0; i < urlList.size(); ++i )
                    {
                        const QString url = urlList.at( i ).path();
                        signalViewAddTrack( url.toStdString() );
                    }
                    if ( urlList.size() )
                    {
                        signalViewStartPlaylist();
                    }
                }
            );
        }
    }
}

void MVPPlayerLocalDialog::editSettings()
{
    MVPPlayerSettingsDialog settingsDialog( this );
    if ( settingsDialog.exec() )
    {
        
    }
}

void MVPPlayerLocalDialog::dragEnterEvent( QDragEnterEvent *event )
{
    event->acceptProposedAction();
}

void MVPPlayerLocalDialog::dragMoveEvent( QDragMoveEvent *event )
{
    event->acceptProposedAction();
}

void MVPPlayerLocalDialog::dragLeaveEvent( QDragLeaveEvent *event )
{
    event->accept();
}

void MVPPlayerLocalDialog::startStopServer( const bool start )
{
    if ( start )
    {
        signalViewStartServer();
    }
    else
    {
        signalViewStopServer();
    }
}

void MVPPlayerLocalDialog::slotSetIconStop()
{
    _btnPlayPause->setChecked( true );
}

void MVPPlayerLocalDialog::slotSetIconPlay()
{
    _btnPlayPause->setChecked( false );
}

void MVPPlayerLocalDialog::slotSetPlaylistItemIndex( const int row )
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks

    if ( row == -1 )
    {
        widget.playlist->selectionModel()->clearSelection();
    }
    else if ( row < widget.playlist->count() )
    {
        widget.playlist->setCurrentRow( row );
    }

    widget.playlist->blockSignals( false );
}

QString MVPPlayerLocalDialog::slotOpenFile( const QString & title, const QString & extensions )
{
    return QFileDialog::getOpenFileName( QApplication::activeWindow(), title, QDir::currentPath(), extensions );
}

void MVPPlayerLocalDialog::slotOpenedPlaylist( const QStringList & filenames )
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->clear();
    widget.playlist->addItems( filenames );
    widget.playlist->blockSignals( false );
    signalViewStartPlaylist();
}

void MVPPlayerLocalDialog::slotClearPlaylist()
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->clear();
    widget.playlist->blockSignals( false );
}

void MVPPlayerLocalDialog::slotAddTrack( const QString & filename )
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->addItem( filename );
    widget.playlist->blockSignals( false );
}

void MVPPlayerLocalDialog::slotDisplayError( const QString & msg )
{
    QMessageBox::critical( QApplication::activeWindow(), QObject::tr( "Error!" ), msg, QMessageBox::Ok | QMessageBox::Default);
}

}
}
}
