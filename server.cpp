#include<iostream>
#include<list>
#include<map>
#include<queue>
#include<cstdlib>
#include<string>
#include<ctime>
#include<boost/asio.hpp>
#include<boost/thread.hpp>
#include<boost/asio/ip/tcp.hpp>
#include<fstream>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
using std::string;
using std::map;
typedef std::shared_ptr<tcp::socket> socket_ptr;
typedef std::shared_ptr<string> string_ptr;
typedef map<socket_ptr, string_ptr> clientMap;
typedef std::shared_ptr<clientMap> clientMap_ptr;
typedef std::shared_ptr< list<socket_ptr> > clientList_ptr;
typedef std::shared_ptr< queue<clientMap_ptr> > messageQueue_ptr;

io_service service;
tcp::acceptor acceptor(service, tcp::endpoint(tcp::v4(), 8001));
boost::mutex mtx;
clientList_ptr clientList(new list<socket_ptr>);
messageQueue_ptr messageQueue(new queue<clientMap_ptr>) ;
map<socket_ptr, string> dict; 

const int bufSize = 2048;


// Function Declaration
bool clientSentExit(string_ptr);
void disconnectClient(socket_ptr);
void acceptorLoop();
void requestLoop();
void responseLoop();
void FIFOexecute();
void SJFexecute();
void Priorityexecute();
void Roundexecute();

class job
{
   private : string client = "";
             int jobid = 0;
             int priority = 0;
             int length = 0;             
             int endtime = 0;
             int starttime = 0;
             double waitingtime = 0.0;
             int runtime = 0;
   public: job(string x, int y, int z, int w)
       {
              client = x;
              jobid = y;
              priority = z;
              length = w;
              runtime = w;
       }
           job()
       {
             client = "";
             jobid = 0;
             priority = 0;
             length = 0;
       }
       int idgetter()
       {
           return jobid;
       }
       int prioritygetter()
       {
            return priority;
       }
       int lengthgetter()
       {
           return length;
       }
       string clientgetter()
       {
          return client;
       }
       void endsetter(int x)
       {
          endtime = x;
       }
       int endgetter()
       {
            return endtime;
       }
       void startsetter(int x)
       {
           starttime = x;
       }
       int startgetter()
       {
           return starttime;
       }
       void waitingsetter(double x)
       {
           waitingtime = x;
       }
       double waitinggetter()
       {
            return waitingtime;
       }
       int runtimegetter()
       {
         return runtime;
       }
       void runtimesetter(int x)
       {
         runtime = x;
       }
};

struct prioritykey
{
  inline bool operator() ( job &one, job &two)
  {
      return (one.prioritygetter() > two.prioritygetter());
  }
};

struct lengthkey
{
  inline bool operator() ( job &one,  job &two)
  {
      return (one.lengthgetter() < two.lengthgetter());
  }
};

vector<job> fifo;
vector<job> sjf;
vector<job> priorityqueue;
vector<job> roundrobin; 


int main(int argc, char** argv)
{
   // ofstream myfile1;
   // ofstream myfile2;
   // ofstream myfile3;
   // myfile1.open("fifo");
   // myfile2.open("sjf");
   // myfile3.open("priorityqueue"); 
    boost::thread_group threads;

    threads.create_thread(boost::bind(FIFOexecute));
    boost::this_thread::sleep( boost::posix_time::millisec(100));

    threads.create_thread(boost::bind(SJFexecute));
    boost::this_thread::sleep( boost::posix_time::millisec(100));

    threads.create_thread(boost::bind(Priorityexecute));
    boost::this_thread::sleep( boost::posix_time::millisec(100));

    threads.create_thread(boost::bind(Roundexecute));
    boost::this_thread::sleep( boost::posix_time::millisec(100));

    threads.create_thread(boost::bind(acceptorLoop));
    boost::this_thread::sleep( boost::posix_time::millisec(100));
    
    threads.create_thread(boost::bind(requestLoop));
    boost::this_thread::sleep( boost::posix_time::millisec(100));
    
    threads.create_thread(boost::bind(responseLoop));
    boost::this_thread::sleep( boost::posix_time::millisec(100));
    
    threads.join_all();
    
    puts("Press any key to continue...");
    getc(stdin);
    return EXIT_SUCCESS;
}

void acceptorLoop()
{
    cout << "Waiting for clients..." << endl;
    
    for(;;)
    {
        socket_ptr clientSock(new tcp::socket(service));
        
        acceptor.accept(*clientSock);
        
        cout << "New client joined! ";
        
        mtx.lock();
        clientList->emplace_back(clientSock);
        mtx.unlock();
        
        cout << clientList->size() << " total clients" << endl;
    }
}

void requestLoop()
{
    for(;;)
    {
        if(!clientList->empty())
        {
            mtx.lock();
            for(auto& clientSock : *clientList)
            {
                if(clientSock->available())
                {
                    char readBuf[bufSize] = {0};
                    
                    size_t bytesRead = clientSock->read_some(buffer(readBuf, bufSize));
                    
                    string_ptr msg(new string(readBuf, bytesRead));
                    
                    if(clientSentExit(msg))
                    {
                        disconnectClient(clientSock);
                        break;
                    }
                    if(msg->find("msg") != string::npos)
                    {
                    clientMap_ptr cm(new clientMap);
                    string temp = *msg;
                    temp = temp.substr(3);
                    *msg = temp;
                    if (dict.find(clientSock) == dict.end())
                    {
                         string clientname = "";
                         int i = 0;
                         while(temp[i] != ':')
                            clientname += temp[i++];
                         dict[clientSock] = clientname;
                    }
                    cm->insert(pair<socket_ptr, string_ptr>(clientSock, msg));                    
                    messageQueue->push(cm);                    
                    cout << "ChatLog: " << *msg << endl;
                    }
                    else if(msg->find("job") != string::npos)
                    {
                       string temp = *msg;
                       temp = temp.substr(3);
                       int i = 0;
                       while (temp[i] != ':')
                       ++i;
                       string client = temp.substr(0,i);
                        if (dict.find(clientSock) == dict.end())
                         dict[clientSock] = client;
                       //cout << client << endl;
                       temp = temp.substr(i + 3);
                       i = 0;
                       while (temp[i] != ' ')
                       ++i;
                       string ids = temp.substr(0,i);
                       //cout << ids << endl;
                       temp = temp.substr(i + 1);
                       i = 0;
                       while (temp[i] != ' ')
                       ++i;
                       string lengths = temp.substr(0,i);
                       //cout << lengths << endl;
                       temp = temp.substr(i + 1);
                       i = 0;
                       while (temp[i] != ' ')
                       ++i;
                       string prioritys = temp.substr(0,i);
                       //cout << prioritys << endl;
                       int id = stoi(ids.c_str());
                       int priority = stoi(prioritys.c_str());
                       int length = stoi(lengths.c_str());
                       clock_t start = clock();
                       job newjob1 = job(client,id,priority,length);
                       newjob1.startsetter(start);
                       job newjob2 = job(client,id,priority,length);
                       newjob2.startsetter(start);
                       job newjob3 = job(client,id,priority,length);
                       newjob3.startsetter(start);
                       job newjob4 = job(client,id,priority,length);
                       newjob4.startsetter(start);
                       fifo.push_back(newjob1);
                       sjf.push_back(newjob2);
                       priorityqueue.push_back(newjob3);
                       roundrobin.push_back(newjob4);
                       //cout << "we finish receiving one job" << client << " " << ids << " " << prioritys << " " << lengths << endl; 
                    }
                }
            }
            mtx.unlock();
        }
        
        boost::this_thread::sleep( boost::posix_time::millisec(200));
    }
}

bool clientSentExit(string_ptr message)
{
    if(message->find("exit") != string::npos)
        return true;
    else
        return false;
}

void disconnectClient(socket_ptr clientSock)
{
     for (std::map<socket_ptr, string>:: iterator it = dict.begin(); it != dict.end(); ++it)
     {
        if (it->first == clientSock)
          {
             cout << "Client " << it->second << " is going to disconnect" << endl;
             dict.erase(it);
             break;
          }
      }            
    auto position = find(clientList->begin(), clientList->end(), clientSock);
    
    clientSock->shutdown(tcp::socket::shutdown_both);
    clientSock->close();
    
    clientList->erase(position);
    
    cout << "Client Disconnected! " << clientList->size() << " total clients" << endl;
}

void responseLoop()
{
    for(;;)
    {
        if(!messageQueue->empty())
        {
            auto message = messageQueue->front();
            
            mtx.lock();
            for(auto& clientSock : *clientList)
            {
                clientSock->write_some(buffer(*(message->begin()->second), bufSize));
               // cout << "one message writen successfully" << endl;
            }
            mtx.unlock();
            
            mtx.lock();
            messageQueue->pop();
            //cout << "one message pop out" << endl;
            mtx.unlock();
        }
        
        boost::this_thread::sleep( boost::posix_time::millisec(200));
    }
}

void FIFOexecute()
{
  ofstream myfile1;
  myfile1.open("fifo");
  job  x = job();
  for(;;)
  {
     
     if (x.clientgetter() == "")
     {
          if (fifo.size() < 1)
          boost::this_thread::sleep( boost::posix_time::millisec(10));
          else
          {
            mtx.lock();
            clock_t end = clock();
            x = fifo[0];
            x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            fifo.erase(fifo.begin());
            mtx.unlock();
           boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
          }
     }
     else
     {      mtx.lock();
            //clock_t end = clock();
            //x->endsetter(end);
            //cout << "one job finished " << "start time is " << x->startgetter() << " end time is " << x->endgetter() << endl;
            //double waitingtime =0.0 + 1000000.0 * ((double) (x->endgetter() - x->startgetter())) / CLOCKS_PER_SEC;
            //cout << "Waiting time is " << waitingtime << endl;
            //cout << "sleeptime is " << x->lengthgetter() << endl;
            //x->waitingsetter(waitingtime - x->lengthgetter());
            //cout << "the waiting time is " << x->waitinggetter() << endl;
            //delete x;
            myfile1 << "client name: " << x.clientgetter() << " job ID: " << x.idgetter() << " job Length: " << x.lengthgetter() << " priority: " << x.prioritygetter() << " start time: " << x.startgetter() << " end time: " << x.endgetter() << " waiting time " << x.waitinggetter() << endl;  
            for (std::map<socket_ptr, string>:: iterator it = dict.begin(); it != dict.end(); ++it)
            {
               if (it->second == x.clientgetter())
               {
                    string message = "FIFO Server: ";
                    message += " Job ID: " + std::to_string(x.idgetter());
                    message += " Job Priority: " + std::to_string(x.prioritygetter());
                    message += " Job Length: " + std::to_string(x.lengthgetter());
                    message += "\n";
                   // cm->insert(pair<socket_ptr, string_ptr>(it->first, &message));
                    for(auto& clientSock : *clientList)
                    {
                        if (clientSock == it->first)
                        {
                        clientSock->write_some(buffer(message, bufSize));
                        break;
                        }
                    }
                    break;
               }
            }
            if (fifo.size() < 1)
            {
            x =  job();
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(10));
            }
            else
            {
            clock_t end = clock();
            x = fifo[0];
            x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            fifo.erase(fifo.begin());
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
            }
     }    
  } 
}
void SJFexecute()
{
  ofstream myfile2;
  myfile2.open("sjf");
  job x = job();
  for(;;)
  {
      if (x.clientgetter() == "")
     {
          if (sjf.size() < 1)
          boost::this_thread::sleep( boost::posix_time::millisec(10));
          else
          {
            mtx.lock();
            std::sort(sjf.begin(), sjf.end(), lengthkey());
            clock_t end = clock();
            x = sjf[0];
            x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            sjf.erase(sjf.begin());
            mtx.unlock();
           boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
          }
     }
     else
     {      mtx.lock();
           // clock_t end = clock();
           // x.endsetter(end);
           // cout << "one job finished" << "start time is " << x.startgetter() << " end time is " << x.endgetter() << endl;
            //delete x;
            myfile2 << "Client name: " << x.clientgetter() << " job id: " << x.idgetter() << " job length: " << x.lengthgetter() << " priority: " << x.prioritygetter() << " start time: " << x.startgetter() << " end time: " << x.endgetter() << " waiting time: " << x.waitinggetter() << endl;
            for (std::map<socket_ptr, string>:: iterator it = dict.begin(); it != dict.end(); ++it)
            {
               if (it->second == x.clientgetter())
               {
                    string message = "SJF Server: ";
                    message += " Job ID: " + std::to_string(x.idgetter());
                    message += " Job Priority: " + std::to_string(x.prioritygetter());
                    message += " Job Length: " + std::to_string(x.lengthgetter());
                    message += "\n";
                   // cm->insert(pair<socket_ptr, string_ptr>(it->first, &message));
                    for(auto& clientSock : *clientList)
                    {
                        if (clientSock == it->first)
                        {
                        clientSock->write_some(buffer(message, bufSize));
                        break;
                        }
                    }
                    break;
               }
            }
            if (sjf.size() < 1)
            {
            x = job();
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(10));
            }
            else
            {
            std::sort(sjf.begin(), sjf.end(), lengthkey());
            clock_t end = clock();
            x = sjf[0];
            x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            sjf.erase(sjf.begin());
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
            }
      }
  }
}
void Priorityexecute()
{
   job x = job();
   ofstream myfile3;
   myfile3.open("priorityqueue");
   for(;;)
   {
       if (x.clientgetter() == "")
     {
          if (priorityqueue.size() < 1)
          boost::this_thread::sleep( boost::posix_time::millisec(10));
          else
          {
            mtx.lock();
            std::sort(priorityqueue.begin(), priorityqueue.end(), prioritykey());
            clock_t end = clock();
            x = priorityqueue[0];
             x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            priorityqueue.erase(priorityqueue.begin());
            mtx.unlock();
           boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
          }
     }
     else
     {      mtx.lock();
           // clock_t end = clock();
           // x.endsetter(end);
           // cout << "one job finished" << "start time is " << x.startgetter() << " end time is " << x.endgetter() << endl;
            //delete x;
            myfile3 << "Client name: " << x.clientgetter() << " job id: " << x.idgetter() << " job length: " << x.lengthgetter() << " priority: " << x.prioritygetter() << " start time: " << x.startgetter() << " end time: " << x.endgetter() << " waiting time: " << x.waitinggetter() << endl; 
            for (std::map<socket_ptr, string>:: iterator it = dict.begin(); it != dict.end(); ++it)
            {
               if (it->second == x.clientgetter())
               {
                    string message = "Priority Queue Server: ";
                    message += " Job ID: " + std::to_string(x.idgetter());
                    message += " Job Priority: " + std::to_string(x.prioritygetter());
                    message += " Job Length: " + std::to_string(x.lengthgetter());
                    message += "\n";
                   // cm->insert(pair<socket_ptr, string_ptr>(it->first, &message));
                    for(auto& clientSock : *clientList)
                    {
                        if (clientSock == it->first)
                        {
                        clientSock->write_some(buffer(message, bufSize));
                        break;
                        }
                    }
                    break;
                }
             }
            if (priorityqueue.size() < 1)
            {
            x = job();
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(10));
            }
            else
            {
            std::sort(priorityqueue.begin(), priorityqueue.end(), prioritykey());
            clock_t end = clock();
            x = priorityqueue[0];
             x.endsetter(end);
            double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC;
            x.waitingsetter(waitingtime);
            priorityqueue.erase(priorityqueue.begin());
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(x.lengthgetter())); 
            }
      }
   }
}

void Roundexecute()
{
   job x = job();
   ofstream myfile4;
   myfile4.open("RoundRobin");
   for(;;)
   {
    if (x.clientgetter() == "")
     {
          if (roundrobin.size() < 1)
          boost::this_thread::sleep( boost::posix_time::millisec(10));
          else
          {
            mtx.lock();
            x = roundrobin[0];
            roundrobin.erase(roundrobin.begin());
            mtx.unlock();            
            if (x.runtimegetter() <= 100)
            {
            int temp = x.runtimegetter();
            x.runtimesetter(0);
           boost::this_thread::sleep( boost::posix_time::millisec(temp));
            }
            else 
            {
             x.runtimesetter(x.runtimegetter() - 100);
             boost::this_thread::sleep( boost::posix_time::millisec(100)); 
            } 
           }
       }
    else
       {
           mtx.lock();
           if (x.runtimegetter() == 0)
           {
           clock_t end = clock();
           x.endsetter(end);
           double waitingtime =0.0 + 1000000.0 * ((double) (x.endgetter() - x.startgetter())) / CLOCKS_PER_SEC - x.lengthgetter();
           x.waitingsetter(waitingtime);
            myfile4 << "Client name: " << x.clientgetter() << " job id: " << x.idgetter() << " job length: " << x.lengthgetter() << " priority: " << x.prioritygetter() << " start time: " << x.startgetter() << " end time: " << x.endgetter() << " waiting time: " << x.waitinggetter() << endl; 
            for (std::map<socket_ptr, string>:: iterator it = dict.begin(); it != dict.end(); ++it)
            {
               if (it->second == x.clientgetter())
               {
                    string message = "RoundRobin Algorithm Server: ";
                    message += " Job ID: " + std::to_string(x.idgetter());
                    message += " Job Priority: " + std::to_string(x.prioritygetter());
                    message += " Job Length: " + std::to_string(x.lengthgetter());
                    message += "\n";
                   // cm->insert(pair<socket_ptr, string_ptr>(it->first, &message));
                    for(auto& clientSock : *clientList)
                    {
                        if (clientSock == it->first)
                        {
                        clientSock->write_some(buffer(message, bufSize));
                        break;
                        }
                    }
                    break;
                 }
              }
           }
           else
           roundrobin.push_back(x);
           if (roundrobin.size() < 1)
            {
            x = job();
            mtx.unlock();
            boost::this_thread::sleep( boost::posix_time::millisec(10));
            }
            else
            {
             x = roundrobin[0];
             roundrobin.erase(roundrobin.begin());
             mtx.unlock();
             if (x.runtimegetter() <= 100)
            {
            int temp = x.runtimegetter();
            x.runtimesetter(0);
           boost::this_thread::sleep( boost::posix_time::millisec(temp));
            }
            else 
            {
             x.runtimesetter(x.runtimegetter() - 100);
             boost::this_thread::sleep( boost::posix_time::millisec(100)); 
            }   
          }                  
     }
   }
}
