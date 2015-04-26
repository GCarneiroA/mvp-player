#ifndef _CORE_ISOUNDPLAYER_HPP_
#define	_CORE_ISOUNDPLAYER_HPP_

#include <boost/signals2.hpp>

namespace mvpplayer
{

enum ESeekPosition { eSeekPositionPercent, eSeekPositionMS };

/**
 * Interface for sound players
 */
class ISoundPlayer
{
public:
    ISoundPlayer(): _currentVolume( 1.0f ) {}
    virtual ~ISoundPlayer() {}
    
    /**
     * @brief is the player playing sound
     */
    virtual bool isPlaying() const = 0;

    /**
     * @brief restart track
     * @return false if success, true otherwise
     */
    virtual bool restart() = 0;

    /**
     * @brief sound volume control for the current played track
     * @param v volume value
     */
    virtual void setVolume( const float v ) = 0;

    /**
     * @brief load a given file
     * @param filename given file
     */
    virtual void load( const std::string & filename ) = 0;

    /**
     * @brief plays a sound
     * @param pause pause playing
     * @return false on success, true if error
     */
    virtual bool play( const bool pause = false ) = 0;

    /**
     * @brief stop the current played track
     */
    virtual void stop()
    { unload(); }

    /**
     * @brief stop and free the current played track
     */
    virtual void unload() = 0;

    /**
     * @brief pause or unpause the current track
     * @param pause boolean setting the pause
     */
    virtual void setPause( const bool pause ) = 0;

    /**
     * @brief switch sound off/on
     * @param sound sound on/off
     */
    virtual void setSound( const bool sound ) = 0;

    /**
     * @brief toggle sound on or off
     */
    virtual void toggleSound() = 0;

    /**
     * @brief toggle pause on or off
     */
    virtual void togglePause() = 0;

    /**
     * @brief get current volume
     */
    inline float currentVolume() const
    { return _currentVolume; }

    /**
     * @brief get the current track's position
     * @return the current position in milliseconds
     */
    virtual std::size_t getPosition() const = 0;

    /**
     * @brief get the current track's length
     * @return the current length in milliseconds
     */
    virtual std::size_t getLength() const = 0;

    /**
     * @brief set current track position
     * @param position position in percent (0-100) or ms
     * @param seekType seek position in percent or milliseconds
     */
    virtual void setPosition( const std::size_t position, const ESeekPosition seekType = eSeekPositionPercent ) = 0;

// Signals
public:
    boost::signals2::signal<void()> signalEndOfTrack;                        ///< Signalize end of track
    boost::signals2::signal<void(const std::size_t posMS)> signalTrackLength;      ///< Signalize track length in MS
    boost::signals2::signal<void( const long pos, const long len)> signalPositionChanged;   ///< Signalize track position

protected:
    float _currentVolume;           ///< Current player volume
};

}

#endif
