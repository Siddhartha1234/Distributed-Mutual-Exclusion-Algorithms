#include<mpi.h>
#include<bits/stdc++.h>
#include<fstream>
#include<stdio.h>
#include<ctime>
#include<unistd.h>
#include<random>
#include<atomic>
#include<thread>
#include<string>
#include<queue>
#include<mutex>
#include<sys/time.h>
using namespace std;
int n; // number of nodes
atomic<bool> inCS;
vector<int> quorums;
unordered_map<int,bool> quorumGrants;
vector<bool> terminated;
mutex queueLock;
bool term;
int k; // each process enters CS k times
atomic<int> m_count;
long int initTime;
struct reqMsg
{
  long long int p_id;
  long long int timestamp;
  bool operator<(const reqMsg& rhs) const
  {
    if(timestamp != rhs.timestamp)
      return timestamp > rhs.timestamp;
    else
      return p_id > rhs.p_id;
  }
};
priority_queue<reqMsg> requestQueue;
priority_queue<reqMsg> delayedQueue;

long long int timestamp()
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 +  t.tv_usec;
}
char* getTime() // Retuns current time as hours : minute : seconds string
{
	time_t input = time(0);
	struct tm * timeinfo;
	timeinfo = localtime (&input);
	static char output[10];
	sprintf(output,"%.2d:%.2d:%.2d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	return output;
}
vector<int> GenerateQuorums(int nodeId, vector<int> quorum)
{
  int checkFail = false;
  if(!checkFail)
  {
    quorum.push_back(nodeId);
  }
    // leaf node
  if(nodeId > (n+1) && nodeId <n)
  {
    return quorum;
  }
  else
  {
    vector<int> leftPath = GenerateQuorums( nodeId*2, quorum);
    vector<int> rightPath = GenerateQuorums( nodeId*2+1, quorum);
    if(checkFail && leftPath.size()!=0 && rightPath.size() !=0)
    {
      leftPath.insert(leftPath.end(),rightPath.begin(), rightPath.end());
      return leftPath;
    }
    else if( !checkFail && leftPath.size() != 0 )
    {
      return leftPath;
    }
    else if( !checkFail && rightPath.size() != 0)
    {
      return rightPath;
    }
    else
    {
      vector<int> a;
      return a;
    }
  }
}
vector<int> GenerateQuorum(int nodeId,vector<int> quorum)
{

  int a = 1;
  while(a<n)
  {
    quorum.push_back(a);
    a = a*2;
  }
  return quorum;

}
void enterCS(int id) // Repeated CS request after some sleep time for every process
{
  // cout << "\n" << "enterCS started for" << id  << endl;
  ofstream log_file("TreeQuorum-log.txt", ios::app);
  for(int i=0;i<k;i++)
  {
    usleep(20); // sleep before CS
    // start time to enter CS
    string s = "p" + to_string(id) + " requests CS  " + " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
    vector<int> a;
    quorums = GenerateQuorum(1,a);
    // quorums = a;
    int l = quorums.size();
    unordered_map<int,bool> quo;
    quorumGrants = quo;
    // cout << "\n" << "Reached here " << id << endl;
    long long int timest = timestamp();
    for(int i=0;i<l;i++)
    {
      quorumGrants[quorums[i]]= false;
      if( quorums[i] != id)
      {
        long long int b[2];
        b[0] = 1;
        b[1] = timest;
        MPI_Send(&b,2,MPI_LONG_LONG_INT, quorums[i], 0, MPI_COMM_WORLD);// send a request message
        //cout << "\n" << b[1] << "is the timestamp to " << quorums[i] << "from " << id << endl << flush;
        m_count++;
      }
      else
      {
        queueLock.lock();
        if(requestQueue.empty())
        {
          quorumGrants[quorums[i]] = true;
          cout << "\n" << "Granted " << id << " CS by " << id << endl << flush;
          requestQueue.push(reqMsg{id,timest});
        }
        else
        {
          if(timest <  requestQueue.top().timestamp || (id < requestQueue.top().p_id && timest ==  requestQueue.top().timestamp ))
          {
            long long int b[2];
            b[0] = 2;
            b[1] = timestamp();
            if(delayedQueue.empty())
              MPI_Send(&b,2,MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send an inquire message
            m_count++;
            delayedQueue.push(reqMsg{id,timest});
          }
          else
          {
            requestQueue.push(reqMsg{id,timest});
          }
        }
        queueLock.unlock();
      }
    }
    // cout << "\n" << "Sent request to quorums by " << id << " at " << timest << endl;
    while(!inCS)
    {
      int a=0;
      for(int i=0;i<l;i++)
      {
        if(quorumGrants[quorums[i]])
          a++;
      }
      if( a == l)
        inCS = true;
      // else if( a == l-1)
      //   cout << "Waiting for 1 more by " << id << endl << flush;
    }
    // Entered CS
    s = "p" + to_string(id) + " entered CS " + " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
    usleep(20); // sleep for CS
    // Begin to Exit CS
    inCS = false;
    timest = timestamp();
    for(int i=0;i<l;i++)
    {
      if(quorums[i] != id)
      {
        long long int b[2];
        b[0] = 4;
        b[1] = timest;
        MPI_Send(&b,2,MPI_LONG_LONG_INT, quorums[i], 0, MPI_COMM_WORLD);// send a release message
        // cout << "\n" << b[1] << "is the release timestamp to " << quorums[i] << "from " << id << endl << flush;
        m_count++;
      }
      else
      {
        requestQueue.pop();
        while(!delayedQueue.empty())
        {
          requestQueue.push(delayedQueue.top());
          delayedQueue.pop();
        }
        if(!requestQueue.empty())
        {
          long long int b[2];
          b[0] = 0;
          b[1] = timestamp();
          MPI_Send(&b,2, MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
          cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
          m_count++;
        }
      }
    }
    // Exit CS msg
    s = "p" + to_string(id) + " exits CS " +  " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
  }
  //Send Terminate message from this process perspective to each process
  for(int i=1;i<=n;i++)
  {
    if(i!= id)
    {
      long long int b[2];
      b[0] = 5;
      b[1] = timestamp();
      MPI_Send(&b,2,MPI_LONG_LONG_INT, i, 0, MPI_COMM_WORLD);// send a release message
      m_count++;
    }
    else
      terminated[i] = true;
  }
  log_file.close();
  cout << "p" << id << " has completed its CS needs" << endl << flush;
  while(!term);

  int a = m_count+1; // send message complexity to initializing node
  MPI_Send(&a,1, MPI_INT, 0, 0 , MPI_COMM_WORLD);
  cout << "p" << id << " has reached termination" << endl << flush;
}
void recvMsg(int id, int p_id) // Receiving messages by process 'id'  from process 'p_id'
{
  // cout << "\n" << "recvmsg started " << id << "   " << p_id << endl<< flush;
  while(true)
  {
    long long int a[2];
    MPI_Recv(&a,2, MPI_LONG_LONG_INT, p_id,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    cout << "\n" << "received " << a[0] << "     " << a[1] << " in " << id << "   " << p_id << endl << flush;
    // if grant
    if(a[0] == 0)
    {
      quorumGrants[p_id] = true;
    }
    // if request
    else if(a[0] == 1)
    {
      if(requestQueue.empty())
      {
        queueLock.lock();
        requestQueue.push(reqMsg{p_id,a[1]});
        long long int b[2];
        b[0] = 0;
        b[1] = timestamp();
        MPI_Send(&b,2, MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
        cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
        m_count++;
        queueLock.unlock();
      }
      else if(a[1] <  requestQueue.top().timestamp || (p_id < requestQueue.top().p_id && a[1] ==  requestQueue.top().timestamp ))
      {
        long long int b[2];
        b[0] = 2;
        b[1] = timestamp();
        if(requestQueue.top().p_id == id )
        {
          if(!inCS)
          {
            quorumGrants[id] = false;
            queueLock.lock();
            while(!delayedQueue.empty())
            {
              requestQueue.push(delayedQueue.top());
              delayedQueue.pop();
            }
            requestQueue.push(reqMsg{p_id,a[1]});
            long long int b[2];
            b[0] = 0;
            b[1] = timestamp();
            MPI_Send(&b,2, MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
            cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
            m_count++;
            queueLock.unlock();
          }
          else
          {
            queueLock.lock();
            delayedQueue.push(reqMsg{p_id,a[1]});
            queueLock.unlock();
          }
        }
        else
        {
          queueLock.lock();
          if(delayedQueue.empty())
            MPI_Send(&b,2,MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send an inquire message
          m_count++;
          delayedQueue.push(reqMsg{p_id,a[1]});
          queueLock.unlock();
        }
      }
      else
      {
        queueLock.lock();
        requestQueue.push(reqMsg{p_id,a[1]});
        queueLock.unlock();
      }
    }
    //if inquire
    else if(a[0] == 2)
    {
      if(!inCS)
      {
        if(quorumGrants.count(p_id) != 0)
          quorumGrants[p_id] = false;
        long long int b[2];
        b[0] = 3;
        b[1] = timestamp();
        MPI_Send(&b,2,MPI_LONG_LONG_INT, p_id, 0, MPI_COMM_WORLD);// send a yield message
      }
    }
    // if yield
    else if(a[0] == 3)
    {
      queueLock.lock();
      while(!delayedQueue.empty())
      {
        requestQueue.push(delayedQueue.top());
        delayedQueue.pop();
      }
      queueLock.unlock();
      if(requestQueue.top().p_id != id)
      {
        long long int b[2];
        b[0] = 0;
        b[1] = timestamp();
        MPI_Send(&b,2,MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send a grant  message
        cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
        m_count++;
      }
      else
      {
        quorumGrants[id] = true;
        cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
      }
    }

    // if release
    else if(a[0] == 4)
    {
      if(requestQueue.top().p_id == p_id)
      {
        requestQueue.pop();
        while(!delayedQueue.empty())
        {
          requestQueue.push(delayedQueue.top());
          delayedQueue.pop();
        }
        if(!requestQueue.empty())
        {
          if(requestQueue.top().p_id != id)
          {
            long long int b[2];
            b[0] = 0;
            b[1] = timestamp();
            MPI_Send(&b,2, MPI_LONG_LONG_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
            cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
            m_count++;
          }
          else
          {
            if(quorumGrants.count(id) != 0)
            {
              quorumGrants[id] = true;
              cout << "\n" << "Granted " << requestQueue.top().p_id << " CS by " << id << endl << flush;
            }
          }
        }
      }
    }
    else
    {
      terminated[p_id] = true;
      int i=1;
      while(i<=n)
      {
        if(!terminated[i])
          break;
      }
      if(i==n && terminated[n])
        term = true;
    }
  }

}
void coordProc(int p_id)
{
  int m;
  MPI_Recv(&m,1, MPI_INT, p_id,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  m_count = m_count + m;
  cout << "Node " << p_id << "has return its message complexity" << endl << flush;
}
int main()
{
  int provided;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
  int p_id, p_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &p_id);
  MPI_Comm_size(MPI_COMM_WORLD, &p_size);
  ofstream log_file("TreeQuorum-log.txt",ios::out);
  log_file.close();
  ifstream input_file("inp-params.txt");
  input_file >> n >> k;
  input_file.close();
  time_t t = time(0);
  initTime = static_cast<long int> (t);
  // cout << "\n" << "inputs : " << n << k << endl;
  // cout << "\n" << p_id <<  "   " << p_size << endl;
  if(p_id == 0)
  {
    // cout << "\n" << "Case for " << p_id << endl;
    thread a[n];
    // cout << "\n" << "Starting threads for " << p_id << endl;
    for(int i=0;i<n;i++)
      a[i] = thread(coordProc, i+1);
    // cout << "\n" << "Closing threads for " << p_id << endl;
    for(int i=0;i<n;i++)
      a[i].join();
    // cout << "\n" << "Closed threads for " << p_id << endl;
    ofstream outfile("Message Complexity.txt", ios::out);
    outfile << "Message Complexity : " << m_count << endl;
    outfile.close();
  }
  if(p_id > 0 && p_id <= n)
  {
    // cout << "\n" << "Case for " << p_id << endl;
    vector<bool> b(n+1);
    for(int i=0;i<n+1;i++)
      b[i] = false;
    terminated = b;
    term = false;
    inCS = false;
    thread a[n]; // n-1 receivers, 1 CS thread
    // cout << "\n" << "Starting threads for " << p_id << endl;
    int j = 1;
    for(int i=1;i<=n;i++)
    {
      if(i!=p_id)
      {
        a[j] = thread(recvMsg, p_id, i);
        j++;
      }
    }
    usleep(10000);
    a[0] = thread(enterCS,p_id);
    // cout << "\n" << "Closing threads for " << p_id << endl;
    a[0].join();
    for(int i=0;i<n;i++)
      a[i].detach();
    // cout << "\n" << "Closedthreads for " << p_id << endl;
  }
  cout << "\n" << "Reached end " << p_id << endl;
  return 0;
  MPI_Finalize();
  return 0;
}
