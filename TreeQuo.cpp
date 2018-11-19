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
  int p_id;
  int timestamp;
  bool operator<(const reqMsg& rhs) const
  {
    return timestamp > rhs.timestamp;
  }
};
priority_queue<reqMsg> requestQueue;
priority_queue<reqMsg> delayedQueue;

int timestamp()
{
  time_t t = time(0);
  long int t1 = static_cast<long int> (t);
  return (t1-initTime)%INT_MAX;
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
    vector<int> rightPath = GenerateQuorums( nodeId*2, quorum);
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

void enterCS(int id) // Repeated CS request after some sleep time for every process
{
  ofstream log_file("TreeQuorum-log.txt", ios::app);
  for(int i=0;i<k;i++)
  {
    usleep(20); // sleep before CS
    // start time to enter CS
    string s = "p" + to_string(id) + " requests CS  " + " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
    vector<int> a;
    quorums = GenerateQuorums(1,a);
    int l = quorums.size();
    unordered_map<int,bool> quo;
    quorumGrants = quo;
    for(int i=0;i<l;i++)
    {
      quorumGrants[quorums[i]]= false;
      int b[2];
      b[0] = 1;
      b[1] = timestamp();
      MPI_Send(&b,2,MPI_INT, quorums[i], 0, MPI_COMM_WORLD);// send a request message
      m_count++;
    }
    while(!inCS)
    {
      int a=0;
      for(auto i : quorumGrants)
      {
        if(i.second)
          a++;
        else
          break;
      }
      if( a == l)
        inCS = true;
    }
    // Entered CS
    s = "p" + to_string(id) + " entered CS " + " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
    usleep(20); // sleep for CS
    // Begin to Exit CS
    for(int i=0;i<l;i++)
    {
      int b[2];
      b[0] = 4;
      b[1] = timestamp();
      MPI_Send(&b,2,MPI_INT, quorums[i], 0, MPI_COMM_WORLD);// send a release message
      m_count++;
    }
    // Exit CS
    s = "p" + to_string(id) + " exits CS " +  " at " + string(getTime()) + "\n";
    log_file << s <<  flush;
    inCS = false;
  }
  //Send Terminate message from this process perspective to each process
  for(int i=1;i<=n;i++)
  {
    if(i!= id)
    {
      int b[2];
      b[0] = 5;
      b[1] = timestamp();
      MPI_Send(&b,2,MPI_INT, i, 0, MPI_COMM_WORLD);// send a release message
      m_count++;
    }
    else
      terminated[i] = true;
  }
  log_file.close();
}
void recvMsg(int id, int p_id) // Receiving messages by process 'id'  from process 'p_id'
{
  while(!term)
  {
    int a[2];
    MPI_Recv(&a,2, MPI_INT, p_id,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // if grant
    if(a[0] == 0)
    {
      if(quorumGrants.count(p_id) != 0)
        quorumGrants[p_id] = true;
    }
    // if request
    else if(a[0] == 1)
    {
      if(requestQueue.empty())
      {
        queueLock.lock();
        requestQueue.push(reqMsg{p_id,a[0]});
        int b[2];
        b[0] = 0;
        b[1] = timestamp();
        MPI_Send(&b,2, MPI_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
        m_count++;
        queueLock.unlock();
      }
      else if(a[1] <  requestQueue.top().timestamp)
      {
        int b[2];
        b[0] = 2;
        b[1] = timestamp();
        MPI_Send(&b,2,MPI_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send an inquire message
        m_count++;
        queueLock.lock();
        delayedQueue.push(reqMsg{p_id,a[0]});
        queueLock.unlock();
      }
      else
      {
        queueLock.lock();
        requestQueue.push(reqMsg{p_id,a[0]});
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
        int b[2];
        b[0] = 3;
        b[1] = timestamp();
        MPI_Send(&b,2,MPI_INT, p_id, 0, MPI_COMM_WORLD);// send a yield message
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
      int b[2];
      b[0] = 1;
      b[1] = timestamp();
      MPI_Send(&b,2,MPI_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send an inquire message
      m_count++;
    }

    // if release
    else if(a[0] == 4)
    {
      if(requestQueue.top().p_id == p_id)
      {
        requestQueue.pop();
        if(!requestQueue.empty())
        {
          int b[2];
          b[0] = 0;
          b[1] = timestamp();
          MPI_Send(&b,2, MPI_INT, requestQueue.top().p_id, 0, MPI_COMM_WORLD);// send grant message
          m_count++;
        }
      }
    }
    else
    {
      terminated[p_id] = true;
      int i=1;
      while(i<=n)
      {
        if(!terminated[p_id])
          break;
      }
      if(i==n && terminated[n])
        term = true;
    }
  }
  int a = m_count+1; // send message complexity to initializing node
  MPI_Send(&a,1, MPI_INT, 0, 0 , MPI_COMM_WORLD);
}
void coordProc(int p_id)
{
  int m;
  MPI_Recv(&m,1, MPI_INT, p_id,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  m_count = m_count + m;
}
int main()
{
  int provided;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
  int p_id,n, p_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &p_id);
  MPI_Comm_size(MPI_COMM_WORLD, &p_size);
  ofstream log_file("TreeQuorum-log.txt",ios::out);
  log_file.close();
  ifstream input_file("inp_params.txt");
  input_file >> n >> k;
  input_file.close();
  time_t t = time(0);
  initTime = static_cast<long int> (t);
  if(p_id == 0)
  {
    thread a[n];
    cout << "Starting threads for " << p_id << endl;
    for(int i=0;i<n;i++)
      a[i] = thread(coordProc, i+1);
    for(int i=0;i<n;i++)
      a[i].join();
    ofstream outfile("Message Complexity.txt", ios::out);
    outfile << "Message Complexity : " << m_count << endl;
    outfile.close();
  }
  if(p_id > 0 && p_id <=n)
  {
    vector<bool> t(n);
    for(int i=0;i<n;i++)
      t[i] = false;
    terminated = t;
    term = false;
    inCS = false;
    thread a[n]; // n-1 receivers, 1 CS thread
    cout << "Starting threads for " << p_id << endl;
    a[0] = thread(enterCS,p_id);
    int j = 1;
    for(int i=1;i<=n;i++)
    {
      if(i!=p_id)
      {
        a[j] = thread(recvMsg, p_id, i);
        j++;
      }
    }
    for(int i=0;i<n;i++)
      a[i].join();
  }
  return 0;
  MPI_Finalize();

}
