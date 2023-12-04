#include <iostream>
using namespace std;
#include "Connection.h"
#include "MySQLConnectionPool.h"

int main()
{
	/*
	clock_t begin = clock();
	int n = 10000;
	while (n --)
	{
		Connection conn;
		string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
		conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
		conn.update(sql);
	}
	clock_t end = clock();
	cout << end - begin << "ms" << endl;
	*/
	
	/*
	clock_t begin = clock();
	int n = 10000;
	while (n--)
	{
		MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
		shared_ptr<Connection> ptr = pool->getConnection();
		string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
		ptr->update(sql);
	}
	clock_t end = clock();
	cout << end - begin << "ms" << endl;
	*/

	/*
	clock_t begin = clock();
	Connection conn;
	conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
	thread t1([&]() ->void {
		int n = 1250;
		while (n--)
		{
			Connection conn;
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
			conn.update(sql);
		}
		});

	thread t2([&]() ->void {
		int n = 1250;
		while (n--)
		{
			Connection conn;
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
			conn.update(sql);
		}

		});

	thread t3([&]() ->void {
		int n = 1250;
		while (n--)
		{
			Connection conn;
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
			conn.update(sql);
		}
		});

	thread t4([&]() ->void {
		int n = 1250;
		while (n--)
		{
			Connection conn;
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			conn.connect("127.0.0.1", 3306, "root", "81604152", "info");
			conn.update(sql);
		}
		});
	
	t1.join(), t2.join(), t3.join(), t4.join();
	clock_t end = clock();
	std::cout <<  end - begin << "ms" << std::endl;
	*/

	clock_t begin = clock();
	MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
	thread t1([&]() ->void {
		int n = 1250;
		while (n--)
		{
			MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
			shared_ptr<Connection> ptr = pool->getConnection();
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			ptr->update(sql);
		}
		});

	thread t2([&]() ->void {
		int n = 1250;
		while (n--)
		{
			MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
			shared_ptr<Connection> ptr = pool->getConnection();
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			ptr->update(sql);
		}

		});

	thread t3([&]() ->void {
		int n = 1250;
		while (n--)
		{
			MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
			shared_ptr<Connection> ptr = pool->getConnection();
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			ptr->update(sql);
		}
		});

	thread t4([&]() ->void {
		int n = 1250;
		while (n--)
		{
			MySQLConnectionPool* pool = MySQLConnectionPool::getConnectionPool();
			shared_ptr<Connection> ptr = pool->getConnection();
			string sql = "insert into users (name, age, sex) values ('alan', 20, 'male')";
			ptr->update(sql);
		}
		});

	t1.join(), t2.join(), t3.join(), t4.join();
	clock_t end = clock();
	std::cout << end - begin << "ms" << std::endl;
	return 0;
}
