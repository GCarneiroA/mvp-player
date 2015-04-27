#include "MVPPlayerRemoteDialog.hpp"
#include <mvp-player-core/trackTools.hpp>

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

MVPPlayerRemoteDialog::MVPPlayerRemoteDialog( QWidget *parent )
: Parent( parent )
{
    widget.setupUi(this);

    initDialog( widget );

    connect( _btnPlayPause, SIGNAL( clicked(bool) ), this, SLOT( slotViewHitPlayStopBtn() ) );
    connect( widget.cbMute, SIGNAL( clicked(bool) ), this, SLOT( slotViewHitMute(const bool) ) );
    connect( widget.btnServer, SIGNAL( clicked(bool) ), this, SLOT( connectDisconnectClient( const bool ) ) );
    connect( widget.playlist, SIGNAL( currentRowChanged(int) ), this, SLOT( playPlaylistItemAtIndex(int) ) );
}

MVPPlayerRemoteDialog::~MVPPlayerRemoteDialog()
{
}

void MVPPlayerRemoteDialog::slotViewHitPlayStopBtn()
{
    if ( _btnPlayPause->isChecked() == false )
    {
        signalViewHitButton( "Stop", true );
    }
    else
    {
        signalViewHitButton( "Play", true );
    }
}

void MVPPlayerRemoteDialog::playPlaylistItemAtIndex( const int playlistIndex )
{
    signalViewHitPlaylistItem( playlistIndex );
}

void MVPPlayerRemoteDialog::openedPlaylist( const std::vector<m3uParser::PlaylistItem> & playlistItems )
{
    QStringList filenames;
    for( const m3uParser::PlaylistItem & item: playlistItems )
    {
        filenames.append( item.filename.string().c_str() );
    }
    QMetaObject::invokeMethod( this, "slotOpenedPlaylist", Qt::BlockingQueuedConnection, Q_ARG( QStringList, filenames ) );
}

void MVPPlayerRemoteDialog::dropEvent( QDropEvent *de )
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
                    signalViewAddTrack( urlList.first().path().toStdString() );
                    signalViewStartPlaylist();
                }
            );
        }
        else
        {
            signalSequencial(
                [this, &urlList](){
                    signalViewHitButton( "Stop", true );
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

void MVPPlayerRemoteDialog::dragEnterEvent( QDragEnterEvent *event )
{
    event->acceptProposedAction();
}

void MVPPlayerRemoteDialog::dragMoveEvent( QDragMoveEvent *event )
{
    event->acceptProposedAction();
}

void MVPPlayerRemoteDialog::dragLeaveEvent( QDragLeaveEvent *event )
{
    event->accept();
}

void MVPPlayerRemoteDialog::connectDisconnectClient( const bool start )
{
    if ( start )
    {
        signalViewConnect();
    }
    else
    {
        signalViewDisconnect();
    }
}

void MVPPlayerRemoteDialog::slotViewHitMute( const bool checked )
{
    signalViewMute( checked );
}

void MVPPlayerRemoteDialog::slotSetIconStop()
{
    widget.sliderPosition->setEnabled( true );
    _btnPlayPause->setChecked( true );
}

void MVPPlayerRemoteDialog::slotSetIconPlay()
{
    widget.sliderPosition->setEnabled( false );
    _btnPlayPause->setChecked( false );
}

void MVPPlayerRemoteDialog::slotSetPlaylistItemIndex( const int row )
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

void MVPPlayerRemoteDialog::slotOpenedPlaylist( const QStringList & filenames )
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->clear();
    widget.playlist->addItems( filenames );
    widget.playlist->blockSignals( false );
    signalViewStartPlaylist();
}

void MVPPlayerRemoteDialog::slotClearPlaylist()
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->clear();
    widget.playlist->blockSignals( false );
}

void MVPPlayerRemoteDialog::slotAddTrack( const QString & filename )
{
    widget.playlist->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.playlist->addItem( filename );
    widget.playlist->blockSignals( false );
}

void MVPPlayerRemoteDialog::changeTrackPosition( const int positionInPercent )
{
    signalViewHitTrackPosition( positionInPercent );
}

void MVPPlayerRemoteDialog::slotSetTrackLength( const std::size_t lengthInMS )
{
    widget.lblTrackLength->setText( QString::fromStdString( trackLengthToString( lengthInMS ) ) );
}

void MVPPlayerRemoteDialog::slotSetTrackPosition( const int positionInMS, const int trackLength )
{
    widget.sliderPosition->blockSignals( true ); // Don't forget to put this to avoid dead locks
    widget.sliderPosition->setValue( positionInMS / trackLength );
    widget.sliderPosition->blockSignals( false );
}

}
}
}
