#include <iostream>
#include <queue>
#include <string>
#include <cstdlib>
#include <vector>
#include <ctime>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

typedef std::shared_ptr<tcp::socket> socket_ptr;
typedef std::shared_ptr<string> string_ptr;
typedef std::shared_ptr< queue<string_ptr> > messageQueue_ptr;

io_service service; // Boost Asio io_service object
messageQueue_ptr messageQueue(new queue<string_ptr>); // Message queue
tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001); // TCP socket for connecting to server
const int inputSize = 256; // Maximum size for input buffer
string_ptr promptCpy; // Terminal prompt displayed to chat users

//predefined messages:
vector<string> messageList = {"hello","🐹","👾"};

// Function Declarations
bool isOwnMessage(string_ptr);
void displayLoop(socket_ptr);
void inboundLoop(socket_ptr, string_ptr);
void writeLoop(socket_ptr, string_ptr);
string* buildPrompt();

int main(int argc, char** argv)
{
    try
    {
        boost::thread_group threads;
        socket_ptr sock(new tcp::socket(service));
        
        string_ptr prompt( buildPrompt() );
        promptCpy = prompt;
        
        sock->connect(ep);
        
        cout << "Welcome to the ChatServer\nType \"exit\" to quit" << endl;
        
        threads.create_thread(boost::bind(displayLoop, sock));
        threads.create_thread(boost::bind(inboundLoop, sock, prompt));
        threads.create_thread(boost::bind(writeLoop, sock, prompt));
        
        threads.join_all();
    }
    catch(std::exception& e)
    {
        cerr << e.what() << endl;
    }
    
    puts("Press any key to continue...");
    getc(stdin);
    return EXIT_SUCCESS;
}

string* buildPrompt()
{
    //const int inputSize = 256;
    //char inputBuf[inputSize] = {0};
    char nameBuf[inputSize] = {0};
    string* prompt = new string(": ");
    
    cout << "Please input a new username: ";
    cin.getline(nameBuf, inputSize);
    *prompt = (string)nameBuf + *prompt;
    boost::algorithm::to_lower(*prompt);
    
    return prompt;
}

void inboundLoop(socket_ptr sock, string_ptr prompt)
{
    std::size_t bytesRead = 0;
    char readBuf[1024] = {0};
    
    for(;;)
    {
        if(sock->is_open() && sock->available())
        {
            bytesRead = sock->read_some(buffer(readBuf, inputSize));
            string_ptr msg(new string(readBuf, bytesRead));
            
            messageQueue->push(msg);
            cout << "messages received correctly" << endl;
        }
        
        boost::this_thread::sleep( boost::posix_time::millisec(50));
    }
}

void writeLoop(socket_ptr sock, string_ptr prompt)
{
//    char inputBuf[inputSize] = {0};
//    string inputMsg;
//    
//    for(;;)
//    {
//        cin.getline(inputBuf, inputSize);
//        inputMsg = *prompt + (string)inputBuf + '\n';
//        
//        if(!inputMsg.empty())
//        {
//            sock->write_some(buffer(inputMsg, inputSize));
//        }
//        
//        if(inputMsg.find("exit") != string::npos)
//            exit(1);
//        
//        inputMsg.clear();
//        memset(inputBuf, 0, inputSize);
//    }
    size_t messageCount = messageList.size();
    srand((unsigned int)time(NULL));
    for (;;){
        int number = rand() % 3 + 1;
        if (number == 1)
        {
        /*if (!sock->is_open())
        {
        sock.reset(new tcp::socket(service));
        sock->connect(ep);
        }*/        
        string temp = "msg";
        sock->write_some(buffer(temp + *prompt + messageList[rand() % messageCount]));
        boost::this_thread::sleep( boost::posix_time::millisec(1000));
        }
        else if (number == 2)
        {
        /*if (!sock->is_open())
        {
        sock.reset(new tcp::socket(service));
        sock->connect(ep);
        } */       
        string temp = "job";
        temp += *prompt + " ";
        int length = (rand() % 4 + 1) * 100;        
        int jobID =  rand() % 1000 + 1;
        int priority = rand() % 100 + 1;
        temp += std::to_string(jobID) + " ";
        temp += std::to_string(length) + " ";
        temp += std::to_string(priority) + " ";
        cout << "Job Create: " << " JobId: " << std::to_string(jobID) << " Job Priority: " << std::to_string(priority) << " Job length: " << std::to_string(priority) << endl;
        sock->write_some(buffer(temp));
        boost::this_thread::sleep( boost::posix_time::millisec(1000));
        }
        /*else
        {
        if (sock->is_open())
        {            
        sock->write_some(buffer("exit"));
        boost::this_thread::sleep( boost::posix_time::millisec(1000));
        }
        }*/
    }
}

void displayLoop(socket_ptr sock)
{
    for(;;)
    {       
        if(!messageQueue->empty())
        {
            if(!isOwnMessage(messageQueue->front()))
            {
                cout << "\n" + *(messageQueue->front()) << endl;
            }
            
            messageQueue->pop();
        }
        
        boost::this_thread::sleep( boost::posix_time::millisec(50));
    }
}

bool isOwnMessage(string_ptr message)
{
    if(message->find(*promptCpy) != string::npos)
        return true;
    else
        return false;
}
