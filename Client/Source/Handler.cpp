#include <algorithm>
#include <cassert>

#include "Handler.h"


namespace vasily
{

Handler::Handler()
    : _state(State::DEFAULT),
      _mode(Mode::COMMAND),
      _coorninateSystem(CoordinateSystem::WORLD),
      _coefficient(10'000),
      _mapping(
      {
        { "q", State::FORWARD },		{ "a", State::BACK },
        { "w", State::LEFT },			{ "s", State::RIGHT },
        { "e", State::UP },				{ "d", State::DOWN },
        { "r", State::ROLL_PLUS },		{ "f", State::ROLL_MINUS },
        { "t", State::PITHCH_PLUS },	{ "g", State::PITHCH_MINUS },
        { "y", State::YAW_PLUS },		{ "h", State::YAW_MINUS },
        { "c", State::CIRCLIC },		{ "p", State::PARTIAL },
        { "z", State::HOME },           { "x", State::FROM_FILE },
        { "o", State::POSITIONAL}
      })
{
}

bool Handler::checkChangingMode(const std::string_view letter)
{
    if (letter == "=")
    {
        switch (_mode)
        {
            case Mode::READING:
                setMode(Mode::COMMAND);
                break;

            case Mode::COMMAND:
                setMode(Mode::READING);
                break;

            default:
                assert(false);
                break;
        }

        return true;
    }

    return false;
}

bool Handler::checkChangingCoordinateSysytem(const std::string_view letter)
{
    if (const auto [value, check] = utils::parseCoordinateSystem(letter); check)
    {
        setCoordinateSystem(value);
        return true;
    }

    return false;
}

Handler::State Handler::parseCommand(const std::string_view command)
{
    _data = command;
    std::transform(_data.begin(), _data.end(), _data.begin(),
                   [](char c) { return static_cast<char>(::tolower(c)); });

    const std::string letter = _data.substr(0, 1);

    if (checkChangingCoordinateSysytem(letter))
    {
        return State::COORDINATE_TYPE;
    }

    if (const auto iter = _mapping.find(letter); iter != _mapping.end())
    {
        return iter->second;
    }

    if (!checkChangingMode(letter))
    {
        _printer.writeLine(std::cout, "ERROR 02: Incorrect input data!");
    }

    return State::DEFAULT;
}

ParsedResult Handler::parseDataAfterCommand()
{
    ParsedResult result;

    switch (_state)
    {
        case State::DEFAULT:
            if (_data == "=")
            {
                return result;
            }
            break;

        case State::COORDINATE_TYPE:
            if (_data.size() == 1u && utils::isCorrectNumber(_data))
            {
                return result;
            }
            break;

        case State::FULL_CONTROL:
            break;

        case State::POSITIONAL:
        {
            const long long num = std::count(_data.begin(), _data.end(), '|');
            if (_data.size() > 1 && num >= 2)
            {
                result.points.reserve(num);

                std::size_t prev = 0;

                std::size_t count = 0;

                bool flag = true;

                _data += '|';

                for (std::size_t i = 0; i < _data.size(); ++i)
                {
                    if (_data[i] == '|')
                    {
                        const std::string strToParse = _data.substr(prev, i - prev);

                        prev = i + 1;

                        ++count;

                        bool locFlag = true;

                        if (count >= 2)
                        {
                            result.points.emplace_back(utils::fromString<RobotData>(strToParse,
                                locFlag));
                        }

                        flag &= locFlag;
                    }
                }

                if (flag)
                {
                    return result;
                }
            }
            break;
        }

        case State::CIRCLIC:
            [[fallthrough]];
        case State::PARTIAL:
        {
            const long long num = std::count(_data.begin(), _data.end(), '|');
            if (_data.size() > 1 && num >= 3)
            {
                result.points.reserve(num - 1);

                std::size_t prev = 0;

                std::size_t count = 0;

                bool flag = true;

                _data += '|';

                for(std::size_t i = 0; i < _data.size(); ++i)
                {
                    if(_data[i] == '|')
                    {
                        const std::string strToParse = _data.substr(prev, i - prev);

                        prev = i + 1;

                        ++count;

                        bool locFlag = true;

                        if(count == 2)
                        {
                            result.numberOfIterations = utils::fromString<int>(strToParse, locFlag);
                            if(result.numberOfIterations < 1)
                            {
                                flag = false;
                                break;
                            }
                        }
                        if(count >= 3)
                        {
                            result.points.emplace_back(utils::fromString<RobotData>(strToParse,
                                                                                    locFlag));
                        }

                        flag &= locFlag;
                    }
                }
                
                if (flag)
                {
                    return result;
                }
            }
            break;
        }

        case State::HOME:
            [[fallthrough]];
        case State::FROM_FILE:
            if (_data.size() == 1)
            {
                return result;
            }
            break;

        case State::FORWARD:
            [[fallthrough]];
        case State::BACK:
            [[fallthrough]];
        case State::LEFT:
            [[fallthrough]];
        case State::RIGHT:
            [[fallthrough]];
        case State::UP:
            [[fallthrough]];
        case State::DOWN:
            [[fallthrough]];
        case State::ROLL_PLUS:
            [[fallthrough]];
        case State::ROLL_MINUS:
            [[fallthrough]];
        case State::PITHCH_PLUS:
            [[fallthrough]];
        case State::PITHCH_MINUS:
            [[fallthrough]];
        case State::YAW_PLUS:
            [[fallthrough]];
        case State::YAW_MINUS:
            if (_data.size() > 1)
            {
                if (const int coefficient = utils::stringToInt(_data.substr(1u)); coefficient > 0)
                {
                    result.coefficient = coefficient;
                    setCoefficient(coefficient);
                    return result;
                }

                _state = State::DEFAULT;
            }
            else
            {
                return result;
            }
            break;

        default:
            assert(false);
            break;
    }

    _printer.writeLine(std::cout, "ERROR 01: Incorrect input data after literal!");
    result.isCorrect = false;
    return result;
}

void Handler::parseRawData(const std::string& data, RobotData& robotData)
{
    if (data.empty())
    {
        _state = State::DEFAULT;
        return;
    }

    std::string copiedData = data;

    const std::string_view letter = data.substr(0, 1);

    if (checkChangingMode(letter))
    {
        copiedData = data.substr(1);
    }
    if (data.size() == 1 && checkChangingCoordinateSysytem(letter))
    {
        _state = State::COORDINATE_TYPE;
        return;
    }

    bool flag;
    robotData = utils::fromString<RobotData>(copiedData, flag);

    _state = flag ? State::FULL_CONTROL : State::DEFAULT;
}

void Handler::appendCommand(const std::string_view command, RobotData& robotData)
{
    if (command.empty())
    {
        _state = State::DEFAULT;
        return;
    }

    _state = parseCommand(command);
    _parsedResult = parseDataAfterCommand();

    if (!_parsedResult.isCorrect)
    {
        _state = State::DEFAULT;
    }

    switch (_state)
    {
        case State::DEFAULT:
            break;

        case State::COORDINATE_TYPE:
            break;

        case State::CIRCLIC:
            break;

        case State::PARTIAL:
            break;

        case State::POSITIONAL:
            break;

        case State::HOME:
            break;
        
        case State::FROM_FILE:
            break;

        case State::FORWARD:
            robotData.coordinates.at(X) += _coefficient;
            break;                    
                                      
        case State::BACK:             
            robotData.coordinates.at(X) -= _coefficient;
            break;                    
                                      
        case State::LEFT:             
            robotData.coordinates.at(Y) += _coefficient;
            break;                    
                                      
        case State::RIGHT:            
            robotData.coordinates.at(Y) -= _coefficient;
            break;                    
                                      
        case State::UP:               
            robotData.coordinates.at(Z) += _coefficient;
            break;                    
                                      
        case State::DOWN:             
            robotData.coordinates.at(Z) -= _coefficient;
            break;                        
                                      
        case State::ROLL_PLUS:        
            robotData.coordinates.at(W) += _coefficient;
            break;                    
                                      
        case State::ROLL_MINUS:       
            robotData.coordinates.at(W) -= _coefficient;
            break;                    
                                      
        case State::PITHCH_PLUS:      
            robotData.coordinates.at(P) += _coefficient;
            break;                    
                                      
        case State::PITHCH_MINUS:     
            robotData.coordinates.at(P) -= _coefficient;
            break;                    
                                      
        case State::YAW_PLUS:         
            robotData.coordinates.at(R) += _coefficient;
            break;                    
                                      
        case State::YAW_MINUS:        
            robotData.coordinates.at(R) -= _coefficient;
            break;

        default:
            assert(false);
            break;
    }
}

int Handler::getCoefficient() const noexcept
{
    return _coefficient;
}

void Handler::setCoefficient(const int coefficient) noexcept
{
    _coefficient = coefficient;
}

Handler::State Handler::getCurrentState() const noexcept
{
    return _state;
}

Handler::Mode Handler::getCurrentMode() const noexcept
{
    return _mode;
}

void Handler::setMode(const Mode mode) noexcept
{
    _mode = mode;
}

CoordinateSystem Handler::getCoordinateSystem() const noexcept
{
    return _coorninateSystem;
}

void Handler::setCoordinateSystem(const CoordinateSystem coordninateSystem) noexcept
{
    _coorninateSystem = coordninateSystem;
}

ParsedResult Handler::getParsedResult() const noexcept
{
    return _parsedResult;
}

} // namespace vasily
