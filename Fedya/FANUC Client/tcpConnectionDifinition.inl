#ifndef TCP_CONNECTION_DIFINITION
#define TCP_CONNECTION_DIFINITION
#pragma once

template <typename T>
ConnectionTCP<T>::ConnectionTCP(SOCKET soc, MyQueue<T>* sendQueue, MyQueue<T>* reciveQueue) :
	_soc(soc), _sendQueue(sendQueue), _reciveQueue(reciveQueue),_pocketsRecived("Points from client recived: ",0)
{
}

template <typename T>
void ConnectionTCP<T>::sendCoord()
{
	if (!_sendQueue->empty())
	{
		std::string mes = _sendQueue->front().toString();
		_sendQueue->pop();
		mes += "\n";
		send(_soc, mes.c_str(), static_cast<int>(mes.size()), 0);
	}
}

template <typename T>
int ConnectionTCP<T>::recvCoord()
{
	char buf[256];
	ZeroMemory(buf, 256);
	T rc;

	struct timeval timeout;
	timeout.tv_sec = 100;
	timeout.tv_usec = 0;
	setsockopt(_soc, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

	ZeroMemory(buf, 256);
	const int l = recv(_soc, buf, 255, 0);
	if (l <= 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		return -15;
	}
	if (l > 0)
	{
		_stringBuffer += buf;
		int tmp = 0;
		for (int i = 0; i < _stringBuffer.size() && tmp < 9; ++i)
		{
			if (_stringBuffer[i] != ' ')
			{
				for (; i < _stringBuffer.size() && ((_stringBuffer[i] >= '0' && _stringBuffer[i] <= '9') || _stringBuffer[i] == '-'); ++i);
				if (i < _stringBuffer.size() && _stringBuffer[i] != ' ')
					return _stringBuffer[i];
				tmp++;
			}
			if (tmp == 9)
			{
				sscanf_s(_stringBuffer.c_str(), "%d %d %d %d %d %d %d %d %d",
					&rc._xr, &rc._yr, &rc._zr, &rc._uw, &rc._up, &rc._uz, &rc._segTime, &rc._typeOfMoving, &rc._control);
				_reciveQueue->push(rc);
				_pocketsRecived.setObject(_pocketsRecived.getObject() + 1);
				_stringBuffer = _stringBuffer.substr(i);
				tmp = 0;
				i = 0;
			}
		}
		return 0;
	}
	return 1;
}

#endif