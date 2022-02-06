// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <bits/stdc++.h>
#include <pthread.h>

using namespace std;

struct peer
{
	int client_id;
	int client_port;
	string clientIP;
	string username;
	bool existingUser=false;
	bool login=false;

};
struct peer Client;

struct group
{
	string grp_name;
	
	string grp_owner;
	map<string,int>grp_members;
	map<string,int>grp_pending;
	map<string,int>sharedFiles;
};
struct group C_Group;

struct file
{
	string fname;
	string chunkSHA1;
	string SHA1;
	int chunks;
	int lastChunk;
	vector<int>SourcePeers; //peer_index
};
struct file F;

vector<peer>Peers(10); //contains client sturct
vector<group>Groups;  // contains grp sturct
vector<file>Files;    // contains file struct
map<string,int>GroupInfo; // have info of grpstruct index
map<int,int>PeerInfo;     // have info of peerstruct index
map<string,string>Credentials; // has username password

string StopShare(vector<string>client_command, int c_index)
{
	string reply="";
	string x=client_command[1];
	if(GroupInfo.find(x)==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
	}
	int i=GroupInfo[x];
	if(Groups[i].grp_members.find(Peers[c_index].username)==Groups[i].grp_members.end())
	{
		reply="You are not a member of group";
	}
	else if(Groups[i].sharedFiles.find(client_command[2])==Groups[i].sharedFiles.end())
	{
		reply="File not present";
	}
	else
	{
		int fidx=Groups[i].sharedFiles[client_command[2]];
		auto it= lower_bound(Files[fidx].SourcePeers.begin(),Files[fidx].SourcePeers.end(),c_index);
		Files[fidx].SourcePeers.erase(it);
		if(Files[fidx].SourcePeers.size()==0)
			Groups[i].sharedFiles.erase(client_command[2]);
		reply="File Stopped Sharing";
	}
	return reply;
}

string DownloadFile(vector<string>client_command, int c_index)
{
	string reply="";
	string x=client_command[1];
	if(GroupInfo.find(x)==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
	}
	int i=GroupInfo[x];
	if(Groups[i].grp_members.find(Peers[c_index].username)==Groups[i].grp_members.end())
	{
		reply="You are not a member of group";
	}
	else if(Groups[i].sharedFiles.find(client_command[2])==Groups[i].sharedFiles.end())
	{
		reply="File not present";
	}
	else
	{
		int fidx=Groups[i].sharedFiles[client_command[2]];
		for(auto el:Files[fidx].SourcePeers)
		{
			if(Peers[el].login==true)
				if(Groups[i].grp_members.find(Peers[el].username)!=Groups[i].grp_members.end())
					reply+=to_string(Peers[el].client_port)+" ";
		}
		if(reply.length()==0)
		{
			reply="File not present";
			return reply;
		}
		reply+=Files[fidx].SHA1+" ";
		reply+=Files[fidx].chunkSHA1+" ";
		reply+=to_string(Files[fidx].chunks)+" ";
		reply+=to_string(Files[fidx].lastChunk);
		Files[fidx].SourcePeers.push_back(c_index);
	}
	return reply;
}

string SharableFiles(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=2)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
		return reply;
	}
	else if(GroupInfo.find(client_command[1])==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
	}
	int i=GroupInfo[client_command[1]];
	if(Groups[i].grp_members.find(Peers[c_index].username)==Groups[i].grp_members.end())
	{
		reply="You are not a member of the group.";
	}
	else
	{
		for(auto it:Groups[i].sharedFiles)
		{
			reply.append(it.first);
			reply.append("\n");
		}
	}
	return reply;
}

string UploadFile(vector<string>client_command, int c_index)
{
	string reply="";
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
		return reply;
	}
	else if(GroupInfo.find(client_command[2])==GroupInfo.end())
	{
		reply="No such group exists";
		return reply;
	}
	int i=GroupInfo[client_command[2]];
	if(Groups[i].grp_members.find(Peers[c_index].username)==Groups[i].grp_members.end())
	{
		reply="You are not a member of the group.";
	}
	if(Groups[i].sharedFiles.find(client_command[3])!=Groups[i].sharedFiles.end())
	{
		int fidx=Groups[i].sharedFiles[client_command[3]];
		Files[fidx].SourcePeers.push_back(c_index);
		reply="File Uploaded.";
	}
	else
	{
		int len=Files.size();
		Files.push_back(F);
		Files[len].fname=client_command[3];
		cout<<"name............................."<<Files[len].fname<<endl;
		Files[len].chunkSHA1=client_command[5];
		
		Files[len].SHA1=client_command[4];
		
		Files[len].SourcePeers.push_back(c_index);
		
		Files[len].chunks=stoi(client_command[6]);
		cout<<"chunks..........................."<<Files[len].chunks<<endl;
		Files[len].lastChunk=stoi(client_command[7]);
		cout<<"last.............................."<<Files[len].lastChunk<<endl;
		Groups[i].sharedFiles[client_command[3]]=len;
		reply="File Uploaded.";
	}
	return reply;
}

string Logout(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=1)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
	}
	else
	{
		Peers[c_index].login=false;
		reply="Successfully logged out.";
	}
	return reply;
}

string LeaveGroup(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=2)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
		return reply;
	}
	else if(GroupInfo.find(client_command[1])==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
	}
	int i=GroupInfo[client_command[1]];
	if(Groups[i].grp_members.find(Peers[c_index].username)==Groups[i].grp_members.end())
	{
		reply="You are not a member of the group.";
	}
	else
	{
		if(Groups[i].grp_owner==Peers[c_index].username && Groups[i].grp_members.size()==1)
		{
			Groups.erase(Groups.begin()+GroupInfo[client_command[1]]);
			GroupInfo.erase(client_command[1]);
			reply="Group deleted.";
		}
		else if(Groups[i].grp_owner==Peers[c_index].username)
		{
			Groups[i].grp_members.erase(Peers[c_index].username);
			auto it=Groups[i].grp_members.begin();
			Groups[i].grp_owner=(*it).first;
			reply="Group left.";
		}
		else
		{
			Groups[i].grp_members.erase(Peers[c_index].username);
			reply="Group left.";
		}
	}
	return reply;
}

string AcceptJoiningRequest(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=3)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
		return reply;
	}
	else if(GroupInfo.find(client_command[1])==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
		
	}
	int index=GroupInfo[client_command[1]];
	if(Groups[index].grp_owner!=Peers[c_index].username)
	{
		reply="Access denied.";
	}
	else if(Groups[index].grp_pending.find(client_command[2])==Groups[index].grp_pending.end())
	{
		reply="No such user request found";
	}
	else
	{
		int req_user=Groups[index].grp_pending[client_command[2]];
		Groups[index].grp_members[client_command[2]]=req_user;
		Groups[index].grp_pending.erase(client_command[2]);
		reply="User Request Accepted";
	}
	return reply;
}

string ListPendingJoin(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=2)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
	}
	else if(GroupInfo.find(client_command[1])==GroupInfo.end())
	{
		reply="Group does not exist";
	}
	else if(Groups[GroupInfo[client_command[1]]].grp_owner!=Peers[c_index].username)
	{
		reply="Access denied";
	}
	else
	{
		int index=GroupInfo[client_command[1]];
		for(auto it=Groups[index].grp_pending.begin(); it!=Groups[index].grp_pending.end(); it++)
		{
			reply.append((*it).first);
			reply.append("\n");
		}
	}
	return reply;
}

string JoinGroup(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=2)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
		return reply;
	}
	else if(GroupInfo.find(client_command[1])==GroupInfo.end())
	{
		reply="Group does not exist";
		return reply;
	}
	int index=GroupInfo[client_command[1]];
	if((Groups[index].grp_members).find(Peers[c_index].username)!=Groups[index].grp_members.end())
	{
		reply="Already a member of the group.";
	}
	else if((Groups[index].grp_pending).find(Peers[c_index].username)!=Groups[index].grp_pending.end())
	{
		reply="User already exists in pending requests.";
	}
	else
	{
		Groups[index].grp_pending[Peers[c_index].username]=Peers[c_index].client_id;
		reply="Request sent";
	}
	return reply;
}

string ListAllGroups(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=1)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
	}
	else if(GroupInfo.empty())
	{
		reply="No group present.";
	}
	else
	{
		for(auto it=GroupInfo.begin(); it!=GroupInfo.end(); it++)
		{
			//cout<<(*it).first<<endl;//===========================================to check
			reply.append((*it).first);
			reply.append("\n");
		}
	}
	return reply;
}


string CreateGroup(vector<string>client_command,int c_index)
{
	string reply="";
	if(client_command.size()!=2)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false || Peers[c_index].login==false)
	{
		reply="Please SignUp/LogIn first.";
	}
	else if(GroupInfo.find(client_command[1])!=GroupInfo.end())
	{
		reply="Group name already in use.";
	}
	else
	{
		Groups.push_back(C_Group);
		int pos=Groups.size()-1;
		//cout<<client_command[1];//=================================================to check
		Groups[pos].grp_name=client_command[1];
		//Groups[pos].grp_id=pos;
		//Groups[pos].grp_members.push_back(Peers[c_index].client_id);
		Groups[pos].grp_owner=Peers[c_index].username;
		Groups[pos].grp_members[Peers[c_index].username]=Peers[c_index].client_id;
		GroupInfo[client_command[1]]=pos;
		reply="Group created successfully.";
	}
	return reply;
}

string Login(vector<string>client_command, int c_index)
{
	string reply="";
	//cout<<"log "<<client_command[1]<<" "<<client_command[2]<<endl;
	//cout<<"store "<<Credentials[client_command[1]]<<" "<<Peers[c_index].password<<endl;
	if(client_command.size()!=3)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==false)
	{
		reply="User does not exist";
	}
	else if(Peers[c_index].login==true)
	{
		reply="User already logged in";
	}
	else if(Peers[c_index].username!=client_command[1])
	{
		reply="Username does not match";
	}
	else if(Credentials[client_command[1]]!=client_command[2])
	{
		reply="Incorrect password";
	}
	else
	{
		Peers[c_index].login=true;
		reply="User successfully logged in";
	}
	return reply;
}


string CreateUserAccount(vector<string>client_command, int c_index)
{
	string reply="";
	if(client_command.size()!=3)
	{
		reply="Invalid number of arguments";
		return reply;
	}
	if(Peers[c_index].existingUser==true)
	{
		reply="Already an existing user";
	}
	else if(Credentials.find(client_command[1])!=Credentials.end())
	{
		reply="Username already exists";
	}
	else
	{
		Peers[c_index].existingUser=true;
		Peers[c_index].username=client_command[1];
		Credentials[client_command[1]]=client_command[2];
		reply="User Account Created Successfully";
	}
	return reply;
}


void readCommand(string command, vector<string>&client_command)
{
	istringstream ss(command);
	string cmd;
	while(ss>>cmd)
	{
		client_command.push_back(cmd);
	}
}

void*trackerFunc(void*SocketId)
{
	//to get the info of the client the thread is associated to

	int c_socket=*(int*)SocketId;
	int c_index=PeerInfo[c_socket];
	while(1)
	{
		//now read the commands from client
		char buffer[16384]={0};
		int valread = read( c_socket , buffer, 16384);
		string command=buffer;
		if(command.length()==0)
			continue;
		vector<string>client_command;
		//client_command.clear();
		readCommand(command,client_command);
		//now identify the command
		
		string result="Error";
		//create_user <user_id> <passwd>
		if(client_command[0]=="create_user")
		{
			result=CreateUserAccount(client_command,c_index);
		}
		//login <user_id> <passwd>
		else if(client_command[0]=="login")
		{
			result=Login(client_command,c_index);
		}
		//create_group <group_id>
		else if(client_command[0]=="create_group")
		{
			result=CreateGroup(client_command,c_index);
		}
		//list_groups
		else if(client_command[0]=="list_groups")
		{
			result=ListAllGroups(client_command,c_index);
		}
		//join_group <group_id>
		else if(client_command[0]=="join_group")
		{
			result=JoinGroup(client_command,c_index);
		}
		//list_requests <group_id>
		else if(client_command[0]=="list_requests")
		{
			result=ListPendingJoin(client_command,c_index);
		}
		//accept_request <group_id> <user_id>
		else if(client_command[0]=="accept_request")
		{
			result=AcceptJoiningRequest(client_command,c_index);
		}
		//leave_group <group_id>
		else if(client_command[0]=="leave_group")
		{
			result=LeaveGroup(client_command,c_index);
		}
		//logout
		else if(client_command[0]=="logout")
		{
			result=Logout(client_command,c_index);
		}
		//upload_file <file_path> <group_id >
		else if(client_command[0]=="upload_file")
		{
			result=UploadFile(client_command,c_index);
		}
		else if(client_command[0]=="list_files")
		{
			result=SharableFiles(client_command,c_index);
		}
		else if(client_command[0]=="download_file")
		{
			result=DownloadFile(client_command,c_index);
		}
		//stop_share <group_id> <file_name>
		else if(client_command[0]=="stop_share")
		{
			result=StopShare(client_command,c_index);
		}
		else if(client_command[0]=="close")
		{
			break;
		}
		cout<<result<<endl;
		char*response = new char[result.length() + 1];
    	strcpy(response,result.c_str());
    	send(c_socket, response, strlen(response), 0);
	}
	
}

void * toQuit(void *n) {
    while(1) 
	{
        string input;
        cin>>input;
        if(input == "quit")
            exit(0);
    }
    return n;
}

int main(int argc, char const *argv[])
{
	if(argc!=3)
	{
		cout<<"Invalid Number of Arguments";
		return 0;
	}

	pthread_t qt;
	pthread_create(&qt, NULL, toQuit, NULL);

	string tinfo=argv[1];
    fstream tfile(tinfo,ios::in);
    vector<string> TINFO;
    string temp;
    while(getline(tfile,temp,' ')){
        TINFO.push_back(temp);
    }
    string ip=TINFO[0];
    int tracker_port=stoi(TINFO[1]);
    tfile.close();

	int server_fd, client_socket, valread;
	struct sockaddr_in Addr;
	int opt = 1;
	int addrlen = sizeof(Addr);
	char buffer[1024] = {0};
	//char hello[] = "Hello from server";
	
	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the tracker_port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = INADDR_ANY;
	Addr.sin_port = htons( tracker_port );
	
	// Forcefully attaching socket to the tracker_port 8080
	if (bind(server_fd, (struct sockaddr *)&Addr,sizeof(Addr))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	cout<<"binding done at "<<tracker_port<<endl;
	if (listen(server_fd, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	int index=0;
	while(1)
	{
		cout<<"listening....."<<endl;
		if ((client_socket = accept(server_fd, (struct sockaddr *)&Addr,(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		
		//i got client ip, tracker_port, id in client_socket, therefore create a client struct
		//now in a client struct type vector store this structure
		//and create a map to store mapping of client_socket to index of client vector
		//then create thread to run the functions
		Peers[index].client_id=client_socket;
		Peers[index].clientIP=to_string(Addr.sin_addr.s_addr);//==============check line
		PeerInfo[client_socket]=index;
		//now i will get the port number of the client as a message from client
		valread = read( client_socket , buffer, 1024);
		string c_port=buffer;//======================check line
		Peers[index].client_port=stoi(c_port);//======================check line
		cout<<"Connection established with : "<<Peers[index].client_port<<endl;
		pthread_t newThread;
		if (pthread_create(&newThread, NULL, trackerFunc, (void *)&client_socket) < 0)
        {
            perror("\nError in thread creation\n");
        }
		index++;
	}
	
	return 0;
}
