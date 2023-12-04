
#include <iostream>
#include "MySQLConnectionPool.h"
#include "public.h"

MySQLConnectionPool* MySQLConnectionPool::getConnectionPool()
{
	static MySQLConnectionPool pool; // lock unlock
	return &pool;
}

bool MySQLConnectionPool::loadConfigFile()
{
	FILE* pf;
	fopen_s(&pf, "mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) { continue;}

		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

MySQLConnectionPool::MySQLConnectionPool()
{
	if (!loadConfigFile())
	{
		return;
	}
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime(); 
		_connectionQue.push(p);
		_connectionCnt++;
	}

	thread produce(std::bind(&MySQLConnectionPool::produceConnectionTask, this));
	produce.detach();

	thread scanner(std::bind(&MySQLConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

void MySQLConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
        // queue队列中的连接不为空 当前线程进入阻塞状态
		while (!_connectionQue.empty())
		{
			cv.wait(lock); 
		}
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}
		cv.notify_all();
	}
}
shared_ptr<Connection> MySQLConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("getConnection Timeout !");
				return nullptr;
			}
		}
	}

    // lambda 实现shared_ptr删除器
	shared_ptr<Connection> ptr(_connectionQue.front(),
		[&](Connection* conn)->void {
			unique_lock<mutex> lock(_queueMutex);
            conn->refreshAliveTime();
			_connectionQue.push(conn);
		});

    _connectionQue.pop();
    if (_connectionQue.empty()) cv.notify_all();
	return ptr;
}

void MySQLConnectionPool::scannerConnectionTask()
{
	for (;;)
	{

		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveeTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;
			}
			else
			{
				break;
			}
		}
	}
}
