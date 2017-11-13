#ifndef ROBOT_COMUNICATION_DIFINTION
#define ROBOT_COMUNICATION_DIFINTION
#pragma once



#include <fstream>

#include "MyShower.hpp"
#include "SocketWorking.hpp"
#include "RobotCoord.hpp"

template <typename T>
int RobotConnect<T>::healServerSecondConnection() const
{
	char buffer[128];
	ZeroMemory(buffer, 128);

	SOCKET sockrecv = SocketWorking::getInstance().connectToRobotServer(_serverAddrString.c_str(), _portr,
		_disconnectTime2);

	if (sockrecv == INVALID_SOCKET)
	{
		sprintf_s(buffer, "recive socket error %d\0", WSAGetLastError());
		myInterface::MyShower::getInstance().show(buffer);
		return 1;
	}
	SocketWorking::getInstance().deleteSocket(sockrecv);
	return 0;
}

template <typename T>
int RobotConnect<T>::conRobot()
{
	char buffer[128];

	//�������� ����� ��������
	_sockSend = SocketWorking::getInstance().connectToRobotServer(_serverAddrString.c_str(), _ports, _disconnectTime2);
	if (_sockSend == INVALID_SOCKET)
	{
		sprintf_s(buffer, "send socket error %d\0", WSAGetLastError());
		myInterface::MyShower::getInstance().show(buffer);
		return 1;
	}

	//������� ������ ������
	_sockRecv = SocketWorking::getInstance().connectToRobotServer(_serverAddrString.c_str(), _portr, _disconnectTime2);
	if (_sockRecv == INVALID_SOCKET)
	{
		sprintf_s(buffer, "recive socket error %d\0", WSAGetLastError());
		myInterface::MyShower::getInstance().show(buffer);
		return 2;
	}

	return 0;
}

template <typename T>
void RobotConnect<T>::reverseStream(std::mutex* mt, bool* f, RobotConnect* ins)
{
	long long prevConnetectedTime = clock();
	bool wasFirstPoint = false;
	T rc;
	int was = 1, sleepBonus = 0;
	ins->_connected = true;
	while (true)
	{
		if (ins->_robotRecieve->readCoord() == -15)
		{
			if ((clock() - prevConnetectedTime > ins->_disconnectTime2 + sleepBonus) && wasFirstPoint)
			{
				ins->_forcedRestart = true;
			}
			if ((clock() - prevConnetectedTime > ins->_disconnectTime1 * was + sleepBonus) && wasFirstPoint)
			{
				ins->_connected = false;
				if (ins->_cloneQueue.empty())
				{
					ins->_robotSend->sendPrevCoord();
				}
				else
				{
					sleepBonus = ins->_robotRecieve->_prevCoord.difference(ins->_cloneQueue.front()) / ins->_robotSpeed;
				}
				was++;
			}
		}
		else
		{
			ins->_connected = true;
			prevConnetectedTime = clock();
			was = 1;
			wasFirstPoint = true;
		}
		mt->lock();
		if (!(*f))
		{
			mt->unlock();
			break;
		}
		mt->unlock();
		Sleep(ins->_segtime);
	}
}

template <typename T>
void RobotConnect<T>::sendParalel(std::mutex* mt, bool* f, RobotConnect* ins)
{
	ins->_robotSend->resendCloneQueue();

	while (true)
	{
		if (ins->_connected)
		{
			const std::pair<bool, RobotCoord> tryPull = ins->_sendingQueue->pull();
			if (tryPull.first)
				ins->_robotSend->sendRobotCoord(tryPull.second);//���������� �����
		}
		mt->lock();
		if (!(*f))
		{
			mt->unlock();
			if (ins->_connected)
			{
				while (!ins->_sendingQueue->empty())
				{
					const RobotCoord rc = ins->_sendingQueue->front();
					ins->_sendingQueue->pop();
					ins->_robotSend->sendRobotCoord(rc);//���������� �����
				}
				Sleep(1000);
			}
			break;
		}
		mt->unlock();
		Sleep(ins->_segtime);
	}
}

template <typename T>
int RobotConnect<T>::startWorking()
{
	const int iResult = conRobot();
	if (iResult)
	{
		std::cout << "Conection error " << iResult << " " << WSAGetLastError() << std::endl;
		closesocket(_sockSend);
		closesocket(_sockRecv);
		return -4;
	}
	/// ���������� � ������

	//type of cordiates:0 = JOINT | 1 = JOGFRAME | 2 = WORLDFRAME | 3 = TOOLFRAME | 4 = USER FRAME
	char coord[3] = { 0,0,0 };
	sprintf_s(coord, "%d ", _syscoord);
	if (send(_sockSend, coord, 1, 0) == SOCKET_ERROR)
	{
		std::cout << "Conection error " << WSAGetLastError() << std::endl;
		closesocket(_sockSend);
		return -5;
	}//���������� �����
	 ///����� ������� ���������

	_robotSend = new RobotSend<T>(_sockSend, &_cloneQueue);
	_robotRecieve = new RobotRecieve<T>(_sockRecv, &_cloneQueue, _recivingQueue, _disconnectTime1 / 3);

	_threadSend.startThread(sendParalel, this);

	_threadRecv.startThread(reverseStream, this);

	return 0;
}

template <typename T>
void RobotConnect<T>::tryConnect()
{
	int steps = 0;
	while (startWorking() < 0)
	{
		myInterface::MyShower::getInstance().show("reconecting");
		Sleep(_disconnectTime2);
		myInterface::MyShower::getInstance().update();
		++steps;
		if (steps >= 3)
		{
			healServerSecondConnection();
			steps = 0;
		}
	}
}

template <typename T>
void RobotConnect<T>::finishWorking()
{
	_threadSend.join();
	_threadRecv.join();

	SocketWorking::getInstance().deleteSocket(_sockSend);
	SocketWorking::getInstance().deleteSocket(_sockRecv);

	delete _robotSend;
	delete _robotRecieve;
	_robotSend = nullptr;
	_robotRecieve = nullptr;
}

template <typename T>
void RobotConnect<T>::restart()
{
	finishWorking();

	tryConnect();

	myInterface::MyShower::getInstance().update();
}

template <typename T>
void RobotConnect<T>::mainLoop(std::mutex* mt, bool* f, RobotConnect* ins)
{
	ins->tryConnect();

	while (0 == 0)
	{
		mt->lock();
		if (!*f)
		{
			mt->unlock();
			ins->finishWorking();
			break;
		}
		mt->unlock();

		if (ins->_forcedRestart)
		{
			ins->restart();
			ins->_forcedRestart = false;
		}

		Sleep(ins->_segtime);
	}
}

template <typename T>
RobotConnect<T>::RobotConnect(std::string configFileName, MyQueue<T>* sendingQueue, MyQueue<T>* recivingQueue) :
	_sendingQueue(sendingQueue), _recivingQueue(recivingQueue)
{
	_portr = _disconnectTime1 = _disconnectTime2 = _ports = _robotSpeed = _segtime =
		_syscoord = -1;
	_sockRecv = _sockSend = INVALID_SOCKET;
	_connected = _forcedRestart = false;
	_robotSend = nullptr;
	_robotRecieve = nullptr;

	std::ifstream in(configFileName);
	if (!in)
	{
		myInterface::MyShower::getInstance().show("cannot open file: " + configFileName);
		std::exception exp("cannot open file");
		throw exp;
	}


	in >> _serverAddrString ///��������� IP �������
		>> _ports >> _portr /// �������� ������ ������
		>> _segtime /// ����� �������� �� ����� ����� � ������
		>> _robotSpeed ///��������� �������� ������������ ����� �������
		>> _syscoord /// ��� ������� ��������
		>> _disconnectTime1 /// ����� �����, �������� ����������� �����������
		>> _disconnectTime2; /// ����� ����� �������� ���������� ���������������

	in.close();
}

template <typename T>
void RobotConnect<T>::startMainLoop()
{
	_mainThread.startThread(mainLoop, this);
}

template <typename T>
void RobotConnect<T>::stopMainLoop()
{
	_mainThread.join();
}

template <typename T>
void RobotConnect<T>::restartMainLoop()
{
	stopMainLoop();
	startMainLoop();
}

template <typename T>
RobotConnect<T>::~RobotConnect()
{
	_mainThread.join();
}

#endif