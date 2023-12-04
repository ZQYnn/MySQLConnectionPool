# 项目介绍

为了提高`MySQL`数据库（基于C/S设计）的访问瓶颈，除了在服务器端增加缓存服务器缓存常用的数据 之外（例如`redis`），还可以增加连接池，来提高`MySQL Server`的访问效率，在高并发情况下，大量的 **TCP三次握手`、MySQL Server`连接认证、`MySQL Server`关闭连接回收资源和TCP四次挥手**所耗费的 性能时间也是很明显的，增加连接池就是为了减少这一部分的性能损耗。  

在市场上比较流行的连接池包括阿里的`druid`，`c3p0`以及`apache dbcp`连接池，它们对于短时间内大量 的数据库增删改查操作性能的提升是很明显的，但是它们有一个共同点就是，全部由Java实现的。 

那么本项目就是为了在C/C++项目中，提供`MySQL Server`的访问效率，实现基于C++代码的数据库连接 池模块。

# 功能介绍

连接池一般包含了数据库连接所用的`ip`地址、`port`端口号、用户名和密码以及其它的性能参数，例如初 **始连接量，最大连接量，最大空闲时间、连接超时时间**等，该项目是基于C++语言实现的连接池，主要实现以上几个所有连接池都支持的通用基础功能。

1. **初始连接量(`initSize`)**：表示`MySQL Server`建立初始连接个数, 应用发起`MySQL`访问时，不用再创建和`MySQL Server`新的连接，直接从连接池中获取一个可用的连接 就可以，使用完成后，并不去释放连接，而是把当前连接再归还到连接池当中。
2. **最大连接量(`maxSize`)**: 当并发访问`MySQL Server`请求增多时，在连接池中有存在最大连接量，保证连接池的数量不能超过`maxSize`， 如果无限制创建连接， 服务器端无法接受大量的客户端请求。
3. **最大空闲时间（`maxIdleTime`）**：保证数据库连接池是活跃的， 如果存在连接保存连接池中超过`maxIdleTime`，并且没有被使用到， 那么释放当前连接。
4. **连接超时时间（`connectionTimeout`）**: 当`MySQL`并发请求大， 数据库连接池中连接的个数达到上限，此时如果还有连接请求并且没有空闲的连接可以使用，此时进入阻塞状态，如果获取连接的时间超过`connectionTimeout`，获取连接失败。

# 相关技术

- `MySQL`数据库编程
- 单例模式
- `queue`
- `shared_ptr`
- `lambda` 
- C++11 多线程
-  线程通信同步通信，线程互斥
- `unique_lock()` `CAS`
- 生产者-消费者模型

# 功能设计

实现连接池思路设计

1. 连接池只需要一个实例，所以单例模式进行设计
2. 从`MySQLConnectionPool`中获取连接  实现`getConnection`功能
3. 所有的`Connection` 通过队列保存，使用互斥锁机制保证队列线程安全
4. 控制建立建立连接的数量不能超过`MaxSize`
5. 超过`maxIdleTime` 最大空闲时间， 释放当前连接， 此功能在独立的线程中完成。
6. 获取连接使用智能指针`shared_ptr`进行管理，使用`lambda` 释放连接功能。
7. 多线程编程部分，整体使用生产者-消费者模型， 线程枝间同步机制使用的条件变量和互斥锁。

# 项目模型



![](https://pic-go-oss.oss-cn-beijing.aliyuncs.com/muduo/MySQLConnectionPool.png)



# 开发环境

- `MySQL 5.7`
- `Windows 10` 
- `Visual Studio 2020`

在`Win` 环境下 需要 加载`libmysql.dll`动态连接库，如果是linux 平台下包含`libmysql.so`动态连接库。

实现`MySQL`数据连接需要导入相应的库文件 `MySQL/MySQL Server5.7/include `



# 模块介绍



## 数据库模块

实现MySQL数据库连接，创建数据库表进行测试。

```mysql
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
   	name VARCHAR(50)  ,
    age INT ,
    sex ENUM('male', 'female')
);
```

<img src="https://pic-go-oss.oss-cn-beijing.aliyuncs.com/muduo/sql_table.png" style="zoom:80%;" />



##  连接池对象构造

加载配置文件，创建连接对象，并且启动`produceConnectionTask`与`scannerConnectionTask` 线程。 

```c++
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
```

##  创建连接

```c++
void MySQLConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
    // queue队列中的连接不为空 即不需要创建新连接，当前线程进入阻塞状态
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
```



## 获取连接

```c++
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
  // Conn析构时，再次将conn添加到队列中。
	shared_ptr<Connection> ptr(_connectionQue.front(),
		[&](Connection* conn)->void {
			unique_lock<mutex> lock(_queueMutex);
            conn->refreshAliveTime();
			_connectionQue.push(conn);
		});

    _connectionQue.pop();
   	// 如果当前队列中的连接为空 唤醒produce线程
    if (_connectionQue.empty()) cv.notify_all();
	return ptr;
}
```

## 检测空闲连接

```c++
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
```



# 压力测试

本项目在`Win` 平台下开发， cpu型号`Intel core i7-7700` 测试向MySQL数据库中插入数据多条数据进行测试，测试在使用数据库连接池的情况下节省的时间。

| 数据量 |          未使用连接池           |          使用连接池           |
| :----: | :-----------------------------: | :---------------------------: |
|  1000  |  单线程：3153ms  四线程：898ms  |  单线程:1923ms 四线程：680ms  |
|  5000  | 单线程：14071ms 四线程：3605ms  | 单线程:7316ms  四线程：2869ms |
| 10000  | 单线程：30687ms  四线程：7335ms | 单线程:14882ms 四线程：5693ms |

由此可以得出使用数据连接池可以提高50%以上性能。