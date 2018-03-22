#ifndef CLIENT_H
#define CLIENT_H

#include <chrono>

#include "Handler.h"
#include "Utilities.h"
#include "WinsockInterface.h"   // INCLUDE ORDER DEPENDENCY -> 1 (because of "Windows.h")
#include "TenzoMath.h"          // INCLUDE ORDER DEPENDENCY -> 2


namespace vasily
{

/**
 * \brief Typical realization of client used for interaction with robot server.
 */
class Client : public WinsockInterface
{
public:
    /**
     * \brief Array of modes for client how to work with robot.
     */
    enum class WorkMode
    {
        STRAIGHTFORWARD,
        INDIRECT
    };

    /**
     * \brief Used to define break point for transmitter.
     */
    std::atomic_bool    isNeedToUpdate;

    /**
     * \brief Keep last sent robot's point.
     */
    RobotData           lastSentPoint;


    /**
     * \brief				 Constructor that initializes sockets and connects to server.
     * \param[in] serverPort Server port for connection.
     * \param[in] serverIP	 Server IP address for connection.
     * \param[in] workMode   Set work mode for client to work with robot straightforward or
     *                       indirect.
     */
    explicit	Client(const int serverPort, const std::string_view serverIP,
                       const WorkMode workMode = WorkMode::INDIRECT);

    /**
     * \brief						  Constructor that initializes sockets and connects to server.
     * \param[in] serverSendingPort	  Server port to send.
     * \param[in] serverReceivingPort Server port to recieve.
     * \param[in] serverIP			  Server IP address for connection.
     * \param[in] workMode            Set work mode for client to work with robot straightforward
     *                                or indirect.
     */
    explicit	Client(
        const int serverSendingPort     = _CONFIG.get<int>("DEFAULT_SENDING_PORT_TO_SERVER"), 
        const int serverReceivingPort   = _CONFIG.get<int>("DEFAULT_RECEIVING_PORT_FROM_SERVER"),
        const std::string_view serverIP = _CONFIG.get<std::string>("DEFAULT_SERVER_IP"),
        const WorkMode workMode         = WorkMode::INDIRECT);

    /**	
     * \brief Default destructor.
     */
    virtual     ~Client() noexcept				= default;

    /**
     * \brief			Deleted copy constructor.
     * \param[in] other Other client object.
     */
                Client(const Client& other)		= delete;

    /**
     * \brief			Deleted copy assignment operator.
     * \param[in] other Other client object.
     * \return			Returns nothing because it's deleted.
     */
    Client&		operator=(const Client& other)	= delete;

    /**
     * \brief			 Move constructor.
     * \param[out] other Other client object.
     */
                Client(Client&& other) noexcept;

    /**
     * \brief			 Move assignment operator.
     * \param[out] other Other client object.
     * \return			 Returns an object with all moved data.
     */
    Client&		operator=(Client&& other) noexcept;


    /**
     * \brief  Get server IP address.
     * \return String which contains current server IP address.
     */
    std::string getServerIP() const;

    /**
     * \brief				  Set server IP address.
     * \param[in] newServerIP New server IP address as string.
     */
    void		setServerIP(const std::string_view newServerIP);

    /**
     * \brief  Get current duration.
     * \return Measured time between start point and some event.
     */
    std::chrono::duration<double> getDuration() const;

    /**
     * \brief  Get actual point to interact.
     * \return RobotData structure.
     */
    RobotData   getRobotData() const;

    /**
     * \brief Main method which starts infinite working loop.
     */
    void		run() override;

    /**
     * \brief Fuction processes sockets (call 'connect').
     */
    void		launch() override;

    /**
     * \brief Additional fuction that receives data from server.
     */
    void		receive();

    /**
     * \brief				Check coordinates and if it's right sends to robot.
     * \param[in] robotData Point to send.
     * \return				True if coordinates sent, false otherwise.
     */
    void		sendCoordinates(const RobotData& robotData);

    /**
     * \brief					   Send coordinate system to robot.
     * \param[in] coordinateSystem Coordinate system to send.
     */
    void		sendCoordinateSystem(const CoordinateSystem coordinateSystem) const;

    /**
     * \brief Calibrate strain gauge.
     */
    void        tenzoCalibration();

    /**
     * \brief Do work with strain gauge.
     */
    void        workWithTenzo();


protected:
    /**
     * \brief Array of states to work in circlic mode.
     */
    enum class CirclicState
    {
        SEND_FIRST,
        WAIT_FIRST_ANSWER,
        SEND_SECOND,
        WAIT_SECOND_ANSWER
    };

    /**
     * \brief Structure which contains data that is used for interaction with robot.
     */
    RobotData	_robotData;

    /**
     * \brief Variable used to keep server IP address.
     */
    std::string _serverIP;

    /**
     * \brief Variable used to keep server port.
     */
    int			_serverPort;

    /**
     * \brief Variable used to keep server port to send.
     */
    int			_serverSendingPort;

    /**
     * \brief Variable used to keep server port to recieve.
     */
    int			_serverReceivingPort;

    /**
     * \brief User data handler.
     */
    Handler		_handler;
    
    /**
     * \brief Starting position to measure the time.
     */
    std::chrono::time_point<std::chrono::steady_clock>	_start;

    /**
     * \brief Measured time between start point and some event.
     */
    std::chrono::duration<double>						_duration{};

    /**
     * \brief Data used to send and with we compare answer from robot if it needs.
     */
    RobotData			_waitAnswer;
                            
    /**
     * \brief Flag used to define if client needs to wait answer from robot.
     */
    std::atomic_bool	_isNeedToWait;
                            
    /**
     * \brief Current state of work in circle.
     */
    CirclicState		_circlicState;

    /**
     * \brief Work mode for client, initialize when object created.
     */
    WorkMode            _workMode;

    /**
     * \brief Logger used to write received data to file.
     */
    logger::Logger      _logger;

    /**
     * \brief Class-wrapper used to work with strain gauge. 
     */
    nikita::TenzoMath   _tenzoMath;

    /**
     * \brief Variable used to keep all default parameters and constants.
     */
    static const config::NamedConfig _CONFIG;

    /**
     * \brief Try to establish a connection to a specified socket again.
     */
    void		tryReconnect();

    /**
     * \brief Main infinite working loop. Network logic to interacte with server.
     */
    void		waitLoop() override;

    /**
     * \brief		   Check connection to robot every time.
     * \param[in] time Period time to check.
     */
    void		checkConnection(const long long& time);

    /**
     * \brief						 Work with robot in circlic mode.
     * \details						 Now function works only with 2 points!
     * \param[in] firstPoint		 First point to send and in which robot should return.
     * \param[in] secondPoint		 Second point for circlic movement.
     * \param[in] numberOfIterations Number of iterations in circlic movement.
     * \code
     * Enter command: c|1 2 3 4 5 6 10 2 0|10 20 30 40 50 60 10 2 0|5
     * \endcode
     */
    void		circlicMovement(const RobotData& firstPoint, const RobotData& secondPoint, 
                                const int numberOfIterations = 1);

    /**
     * \brief				    Work with robot in partial mode.
     * \details					Now function works only with 2 points!
     * \param[in] firstPoint	Start point.
     * \param[in] secondPoint	End point.
     * \param[in] numberOfSteps Number of steps for which robot should move from start to end point.
     * \code
     * Enter command: p|1 2 3 4 5 6 10 2 0|10 20 30 40 50 60 10 2 0|3
     * \endcode
    */
    void		partialMovement(const RobotData& firstPoint, const RobotData& secondPoint,
                                const int numberOfSteps = 1);
};

} // namespace vasily

#endif // CLIENT_H
