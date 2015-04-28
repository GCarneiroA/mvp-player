#include "SoundPlayer.hpp"

#include <cassert>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

namespace mvpplayer
{

SoundPlayer::~SoundPlayer()
{
    terminate();
}
    
//initialises sound
void SoundPlayer::initialize()
{
    //create the sound system. If fails, sound is set to impossible
    result = FMOD::System_Create( &_fmodsystem );
    if ( result != FMOD_OK) possible = false;
    //if initialise the sound system. If fails, sound is set to impossible
    if (possible) result = _fmodsystem->init( 2, FMOD_INIT_NORMAL, 0 );
    if (result != FMOD_OK) possible = false;
    createPositionDSP();
}

/**
 * @brief free all
 */
void SoundPlayer::terminate()
{
    signalPositionChanged.disconnect_all_slots();
    signalEndOfTrack.disconnect_all_slots();
    signalTrackLength.disconnect_all_slots();

    unload();

    if ( _dsp )
    {
        _dsp->release();
        _dsp = nullptr;
    }

    if ( _fmodsystem )
    {
        _fmodsystem->close();
        _fmodsystem->release();
        _fmodsystem = nullptr;
    }
}

void SoundPlayer::updater()
{
    assert( _fmodsystem != nullptr );
    try
    {
        while( on )
        {
            boost::mutex::scoped_lock lock( _mutexPlayer );
            if ( _fmodsystem )
            {
                _fmodsystem->update();
            }

            // Wait 25ms
            boost::this_thread::interruption_point();
            boost::this_thread::sleep( boost::posix_time::milliseconds( 25 ) );
            boost::this_thread::interruption_point();
        }
    }
    catch( boost::thread_interrupted& )
    {}
}

//sets the actual playing sound's volume
void SoundPlayer::setVolume( const float v )
{
    _currentVolume = v;
    if ( _channel && v >= 0.0f && v <= 1.0f )
    {
        _channel->setVolume( v );
    }
}

//loads a soundfile
void SoundPlayer::load( const std::string & filename )
{
    _currentSound = filename;
    if ( _fmodsystem )
    {
        if ( on && _sound )
        {
            stop();
        }
        boost::mutex::scoped_lock lock( _mutexPlayer );
        result = _fmodsystem->createStream( _currentSound.c_str(), FMOD_DEFAULT, 0, &_sound );
        possible = ( result == FMOD_OK );
        if ( possible )
        {
            const std::size_t trackLength = getLength();
            signalTrackLength( trackLength );
            signalPositionChanged( 0, trackLength );
        }
    }
}

//frees the sound object
void SoundPlayer::unload()
{
    on = false;
    if ( _updaterThread )
    {
        _mutexPlayer.unlock();
        _updaterThread->join();
    }

    if ( _sound )
    {
        if ( _dsp )
        {
            _channel->removeDSP( _dsp );
        }

        _channel->stop();
        result = _sound->release();
        _sound = nullptr;
        _channel = nullptr;
    }
}

/**
 * @brief restart track
 */
bool SoundPlayer::restart()
{
    if ( possible && on && _channel )
    {
        boost::mutex::scoped_lock lock( _mutexPlayer );
        signalPositionChanged( 0, getLength() );
        return _channel->setPosition( 0, FMOD_TIMEUNIT_MS ) != FMOD_OK;
    }
    return true;
}

//plays a sound (no argument to leave pause as dafault)
bool SoundPlayer::play( const bool pause )
{
    if ( possible && _sound )
    {
        if ( _updaterThread )
        {
            on = false;
            _mutexPlayer.unlock();
            _updaterThread->join();
        }

        {
            boost::mutex::scoped_lock lock( _mutexPlayer );
            result = _fmodsystem->playSound( _sound, NULL, pause, &_channel );
        }
        assert( _channel != NULL );
        _channel->setUserData( this );
        _channel->setCallback( &playEndedCallback );
        on = true;
        setVolume( _currentVolume );
        _channel->addDSP( 0, _dsp );
        // Start fmod updater thread
        // We need to call update every 20 ms to get fmod system status
        _updaterThread.reset( new boost::thread( boost::bind( &SoundPlayer::updater, this ) ) );
        return false;
    }
    return true;
}

//toggles sound on and off
void SoundPlayer::toggleSound()
{
    on = !on;
    if (on == true) { load( _currentSound ); play(); }
    if (on == false) { stop(); }
}

/**
 * @brief pause or unpause the sound
 */
void SoundPlayer::setPause( const bool pause )
{
    signalPositionChanged( getPosition(), getLength() );
    _channel->setPaused( pause );
}

/**
 * @brief get the current track's position
 * @return the current position in milliseconds
 */
std::size_t SoundPlayer::getPosition() const
{
    unsigned int pos = 0;
    _channel->getPosition( &pos, FMOD_TIMEUNIT_MS );
    return pos;
}

/**
 * @brief get the current track's length
 * @return the current length in milliseconds
 */
std::size_t SoundPlayer::getLength() const
{
    unsigned int length = 0;
    _sound->getLength( &length, FMOD_TIMEUNIT_MS );
    return length;
}

//turn sound on or off
void SoundPlayer::setSound( const bool s )
{
    on = s;
}

//toggle pause on and off
void SoundPlayer::togglePause()
{
    bool p = false;
    _channel->getPaused( &p );
    _channel->setPaused( !p );
}

//tells whether the sound is on or off
bool SoundPlayer::getSound()
{
    return on;
}

/**
 * @brief set current track position
 * @param position position in percent (0-100) or ms
 * @param seekType seek position in percent or milliseconds
 */
void SoundPlayer::setPosition( const std::size_t position, const ESeekPosition seekType )
{
    if ( _channel && _sound )
    {
        boost::mutex::scoped_lock lock( _mutexPlayer );
        if ( seekType == eSeekPositionPercent )
        {
            unsigned int length = 0;
            _sound->getLength( &length, FMOD_TIMEUNIT_MS );
            const unsigned int offset = length * ( float( position ) / 100.0f );
            _channel->setPosition( offset, FMOD_TIMEUNIT_MS );
        }
        else
        {
            _channel->setPosition( position, FMOD_TIMEUNIT_MS );
        }
        _fmodsystem->update();
    }
}

/**
 * @brief create dsp used to track the position
 */
void SoundPlayer::createPositionDSP()
{
    assert( _fmodsystem );
    
    if ( _dsp )
    {
        _dsp->release();
        _dsp = nullptr;
    }

    FMOD_DSP_DESCRIPTION dspdesc; 
    memset(&dspdesc, 0, sizeof(dspdesc));

    strncpy( dspdesc.name, "DSP for position", sizeof(dspdesc.name) );
    dspdesc.version = 0x00010000;
    dspdesc.numinputbuffers = 1;
    dspdesc.numoutputbuffers = 1;
    dspdesc.read = &positionCallback;
    dspdesc.userdata = (void *)this;

    _fmodsystem->createDSP( &dspdesc, &_dsp ); 
} 


FMOD_RESULT playEndedCallback(FMOD_CHANNELCONTROL *cchannelcontrol, FMOD_CHANNELCONTROL_TYPE controltype, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype, void *commanddata1, void *commanddata2)
{
    FMOD::ChannelControl *channelcontrol = (FMOD::ChannelControl *)( cchannelcontrol );
    mvpplayer::SoundPlayer* player;
    channelcontrol->getUserData( (void**)( &player ) );
    if ( player )
    {
        switch( controltype )
        {
            case FMOD_CHANNELCONTROL_CALLBACK_END:
            {
                if ( player->isPlaying() )
                {
                    channelcontrol->setCallback( nullptr );
                    player->signalEndOfTrack();
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        return FMOD_ERR_INTERNAL;
    }
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK positionCallback( FMOD_DSP_STATE *dspState, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels ) 
{
    assert( dspState );
    FMOD::DSP *thisDSP = (FMOD::DSP *)dspState->instance;
    mvpplayer::SoundPlayer* player = nullptr;
    thisDSP->getUserData( (void**)( &player ) );

    // Signal track position
    player->signalPositionChanged( player->getPosition(), player->getLength() );
    return FMOD_OK;
}

}
