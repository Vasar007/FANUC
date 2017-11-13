#pragma once

#ifndef MY_SHOWER

#define MY_SHOWER
#include "Field.hpp"
#include <map>
#include <mutex>

namespace myInterface 
{
	//�������� ����������
	class MyShower
	{
		std::map<int, Field> _list;
		int _pos;
		HANDLE _hConsole;
		std::mutex _mt;
		int _stringNumber;

		MyShower()
		{
			_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			_pos = 0;
			_stringNumber = 0;
		}
		
		MyShower(const MyShower&)
		{
			_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			_pos = 0;
			_stringNumber = 0;
		}
		
		MyShower& operator = (MyShower &)
		{
			_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			_pos = 0;
			_stringNumber = 0;
			return getInstance();
		}

	public:

		//����� ���������� ������ � ���������
		int addField(const Field& field)
		{
			_mt.lock();
			_list[++_pos] = field;
			_mt.unlock();
			update();
			return _pos;
		}
		
		//����� ��������������� ����� ���������� 
		void update()
		{
			_mt.lock();
			system("cls");
			_stringNumber = 0;
			for (auto it = _list.begin();it != _list.end();++it, ++_stringNumber)
			{
				it->second.show();
			}
			_mt.unlock();
		}

		//����� ������� ��������� ���������� 
		void updateQiuck()
		{
			COORD coord;
			int i = 0;
			_mt.lock();
			for (auto it = _list.begin();it != _list.end();++it, ++i)
			{
				it->second.showQuick(_hConsole, i);
			}
			coord.X = 0;
			coord.Y = _stringNumber;
			SetConsoleCursorPosition(_hConsole, coord);
			_mt.unlock();
		}

		//����� ������ �������������� ����������
		template<typename T>
		void show(T obj)
		{
			std::cout << obj << "\n";
			++_stringNumber;
		}

		//����� ������������ ���������� ������
		static MyShower& getInstance()
		{
			static MyShower instance;
			return instance;
		}

		//����� �������� ���� �� �������
		void deleteField(int fieldId)
		{
			_mt.lock();
			_list.erase(fieldId);
			_mt.unlock();
			update();
		}

		~MyShower()
		{
			_list.clear();
		}
	};
}

	//*/
#endif