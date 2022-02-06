// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <openssl/sha.h>
#include <bits/stdc++.h>

#define Chunk_Size 524288

using namespace std;



struct clientFile
{
	string name;
	string loc;
	string gid;
	int chunk_count;
	int last_chunk;
	vector<bool> chunk_present;
};
struct clientFile cf;

unordered_map<string,bool>Downloaded;
vector<clientFile> Files;
unordered_map<string, int> FilesInfo;
unordered_map<string,string>SoucrceChunksINFO;
int GtotalSize = 0;

//int PORT;
//int GSock;

bool comparator( pair<int,vector<string>> &a,pair<int,vector<string>> &b)
{
    return (a.second.size() < b.second.size());
}

void readCommand(string command, vector<string> &client_command)
{
	istringstream ss(command);
	string cmd;
	while (ss >> cmd)
	{
		client_command.push_back(cmd);
	}
}

string tracker_comm(string command, int sock)
{
	int val;
	char buff[16384] = {0};
	char*toSend=(char*)malloc(command.length());
	strcpy(toSend, command.c_str());
	fflush(stdout);
	send(sock, toSend, strlen(toSend), 0);
	free(toSend);
	val = read(sock, buff, 16384);
	printf("%s\n", buff);
	string tracker_reply(buff);
	return tracker_reply;
}

string ChunksAvlb(string fname)
{
	cout<<"Inside chunksavl............."<<endl;
	int idx=FilesInfo[fname];
	int i=1;
	string toLeecher="";
	for(auto it:Files[idx].chunk_present)
	{
		if(it)
		{
			toLeecher+=(to_string(i)+" ");
		}
		i++;
	}
	cout<<"Bitmap Info.................."<<toLeecher<<endl;
	return toLeecher;
}

void seeder(int leecher_socket,vector<string>leecher_info)
{
	long long ptr=(stoi(leecher_info[1]))*Chunk_Size;
	int SIZE=stoi(leecher_info[2]);
	char b[SIZE]={0};
	char buff[256]={0};
	int idx=FilesInfo[leecher_info[0]];
	FILE *fp = fopen(Files[idx].loc.c_str(), "r+");
	fseek(fp, ptr, SEEK_SET);

		int bytes_file = fread(b, sizeof(char), SIZE, fp);
		send(leecher_socket, b, bytes_file, 0);
		bzero(b, sizeof(b));
		cout<<".............."<<ptr<<endl;
		fclose(fp);

		read(leecher_socket, buff, 256);
        //in leecher_info <fileName> <chunk no.> <size>
	
}

void*server_thread(void*socket)
{
	cout<<"Entering server_thread................"<<endl;
	int leecher_socket=*(int*)socket;
	char buffer[1024] = {0};
	vector<string>leecher_info;
	int valread = read( leecher_socket , buffer, 1024);
	string leecher=buffer;
	//cout<<"leecher ne server ko diya..............."<<leecher<<endl;
	readCommand(leecher,leecher_info);
	for(auto z:leecher_info)
	{
		cout<<"tokenize......"<<z<<endl;
	}
	if(leecher_info[0]=="Chunks_Available") //<Chunks_Available> <filename>
	{
		//cout<<"OK"<<endl;
		string tl=ChunksAvlb(leecher_info[1]);
		//cout<<"ChunksAvlb se return bhi kr gaya....."<<endl;
		char*response = new char[tl.length() + 1];
    	strcpy(response,tl.c_str());
    	send(leecher_socket, response, strlen(response), 0);
	}
	else
	{
		seeder(leecher_socket,leecher_info);
	}
	//close(leecher_socket);
	return NULL;
}

void *peer_server(void *s_port)
{
	int serverPort = *(int *)s_port;
	int serverSocket, client_socket, valread;
	struct sockaddr_in Addr;
	int opt = 1;
	int addrlen = sizeof(Addr);
	char buffer[1024] = {0};
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				   &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = INADDR_ANY;
	Addr.sin_port = htons(serverPort);

	if (bind(serverSocket, (struct sockaddr *)&Addr, addrlen) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(serverSocket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	cout<<"Client server listening......."<<endl;

	while(1)
	{
		if ((client_socket = accept(serverSocket, (struct sockaddr *)&Addr,(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			return 0;
		}
		cout<<"connected to someone............"<<endl;
		pthread_t newThread;
		if (pthread_create(&newThread, NULL, server_thread, (void *)&client_socket) < 0)
        {
            perror("\nError in thread creation\n");
        }
	}
	close(client_socket);
	close(serverSocket);
}

void*leecher_thread(void*Ccmd)
{
	cout<<"inside leecher..........."<<endl;
	char*tempC=(char*)Ccmd;
	string x;
	for(int i=0; i<strlen(tempC); i++)
	{
		x+=tempC[i];
	}
	cout<<"leecher-x..................."<<x<<endl;
	vector<string>xx;
	readCommand(x,xx);
	x="Chunks_Available "+xx[0];
	cout<<"leecher-command..................."<<x<<endl;
	int sourcePort=stoi(xx[1]);
	int newsock = 0, valread;
	struct sockaddr_in serv_addr;
	//char buffer[1024] = {0};
	if ((newsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return NULL;
	}
	//=======================================Chunk_Available <filename>
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sourcePort);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return NULL;
	}

	if (connect(newsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return NULL;
	}
	cout << "connected to port: " << sourcePort << endl;
	string c=tracker_comm(x,newsock);
	if(c.length()!=0)
		SoucrceChunksINFO[xx[1]]=c;
}

void*finalFile(void*cd)
{
	//cout<<"Entered final..."<<endl;
	//srcPort......destination......filename......chunkNo.......chunksize
	char*tempC=(char*)cd;
	string x;
	for(int i=0; i<strlen(tempC); i++)
	{
		x+=tempC[i];
	}
	vector<string>xx;
	readCommand(x,xx);
	x=xx[2]+" "+xx[3]+" "+xx[4];
	int srcPrt=stoi(xx[0]);
	int chunk_no=stoi(xx[3]);
	int cSize=stoi(xx[4]);
	int byte_read, prev_read=0,newsock;
    char buffer[cSize]={0};

    int pos=chunk_no*Chunk_Size;
	cout<<"pos ptr..."<<pos<<endl;
	cout<<"chunk-1..."<<chunk_no<<endl;
    string path =xx[1]+"/"+xx[2];
	char toSend[x.length()];
	strcpy(toSend, x.c_str());
	struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(srcPrt);


	if ((newsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return NULL;
    }
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return NULL;
	}
    if (connect(newsock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        return NULL;
	cout<<"CONNECTED......"<<endl;
	send(newsock, toSend, strlen(toSend), 0);
    
	FILE *tmp = fopen(path.c_str(), "a");
	fclose(tmp);
	FILE *fd = fopen(path.c_str(), "r+");

	fseek(fd, pos, SEEK_SET);
		while (prev_read < cSize)
		{
			byte_read = recv(newsock, buffer, cSize-prev_read, 0);
			prev_read=prev_read + byte_read;
			fwrite(buffer, sizeof(char), byte_read, fd);
			bzero(buffer, sizeof(buffer));
			cout << "Bytes_Read :" << byte_read << endl;
		}
		fclose(fd);

	write(newsock,"OK",3);
    close(newsock);
    return 0;
}

string findchunkSHA1(string path)
{
	string chunkSHA1 = "";
	vector<string> chunkwise;
	FILE *fp = fopen(path.c_str(), "r+");
	unsigned char buff[Chunk_Size];
	unsigned char csha[20];
	char store[40];
	int r;
	while ((r = fread(buff, 1, sizeof(buff), fp)) > 0)
	{
		SHA1(buff, r, csha);
		buff[Chunk_Size] = {0};
		for (int i = 0; i < 20; i++)
		{
			//chunkSHA1+=csha[i];
			sprintf(store + (2 * i), "%02x", csha[i]);
		}
		chunkSHA1 += store;
		//cout<<store<<"\n";
	}
	cout << "Chunkwise SHA1 : " << chunkSHA1 << endl;
	return chunkSHA1;
}
string findSHA1(string FPATH)
{
	string fileSHA1 = "";
	FILE *fp;
	if ((fp = fopen(FPATH.c_str(), "r+")) == NULL)
	{
		return fileSHA1;
	}
	int totalSize;
	fseek(fp, 0, SEEK_END);
	totalSize = ftell(fp);
	GtotalSize = totalSize;
	rewind(fp);
	string chunkSHA1 = findchunkSHA1(FPATH);
	unsigned char*buff=(unsigned char*)malloc(totalSize*sizeof(unsigned char));
	//cout<<"size of buff-----"<<sizeof(buff)<<endl;
	unsigned char fsha[20];
	char store[40];
	int x = fread(buff, 1, totalSize, fp);
	SHA1(buff, x, fsha);
	free(buff);
	for (int i = 0; i < 20; i++)
	{
		//printf("%02x",fsha[i]);
		sprintf(store + (2 * i), "%02x", fsha[i]);
	}
	//printf("\n");
	fileSHA1 = store;
	cout << "File_SHA1 : " << fileSHA1 << endl;
	return fileSHA1 + " " + chunkSHA1;
}

void DownloadFile(vector<string> client_command, string command, int sock)
{
	SoucrceChunksINFO.clear();
	if (client_command.size() != 4)
	{
		printf("Invalid Number of Arguments");
		return;
	}
	string track_reply=tracker_comm(command,sock);
	if (track_reply == "Group does not exist")
		return;
	else if(track_reply=="You are not a member of group")
		return;
	else if(track_reply=="File not present")
		return;
	else
	{
		int id=Files.size();
		Files.push_back(cf);
		Downloaded[client_command[2]]=false;
		FilesInfo[client_command[2]]=id;
		Files[id].gid=client_command[1];
		Files[id].loc=client_command[3]+"/"+client_command[2];
		
		//<ports>-xTimes <fileSHA>len-4 <chunkSHAs>len-3 <chunkNums>len-2 <lastchunksize>len-1
		vector<string> trackerReply;
		vector<string>SourcePorts;
		
		readCommand(track_reply, trackerReply);
		int length=trackerReply.size();
		int LchunkS=stoi(trackerReply[length-1]);
		int chunkNo=stoi(trackerReply[length-2]);
		string chunkSHAs=trackerReply[length-3];
		string fileSHAs=trackerReply[length-4];
		Files[id].chunk_count=chunkNo;
		Files[id].last_chunk=LchunkS;
		Files[id].chunk_present.assign(Files[id].chunk_count,false);

		for(int i=0; i<length-4;i++)
		{
			SourcePorts.push_back(trackerReply[i]);
			
			//cout<<"SPORT.................."<<trackerReply[i]<<endl;
		}
		//string cmd=client_command[2]+" "+x;
		for(auto x:SourcePorts)
		{
			string cmd=client_command[2]+" "+x;//===============<filename> <Sourceport>
			char Ccmd[1024]={0};
			pthread_t pt;

			//char*Ccmd = (char *) malloc(cmd.length()+1);
			for(int i=0;i<cmd.length();i++)
			{
        		Ccmd[i]=cmd[i];
    		}
			printf("%s",Ccmd);
			printf("\n");
			if (pthread_create(&pt, NULL, leecher_thread, &Ccmd) < 0)
			{
				perror("\nError in thread creation\n");
			}
			//bzero(Ccmd,1024);
			//pthread_detach(pt);
			pthread_join(pt,NULL);
		}
		vector<pair<int,vector<string>>>PieceSel(chunkNo);
		for(auto x:SoucrceChunksINFO)
		{
			cout<<"Port: "<<x.first<<"====="<<x.second<<endl;
			vector<string>PS;
			readCommand(x.second,PS);
			for(auto y:PS)
			{
				int pos=stoi(y)-1;
				PieceSel[pos].first=pos+1;
				PieceSel[pos].second.push_back(x.first);
			}
		}
		for(auto x:PieceSel)
		{
			cout<<"index "<<x.first<<"::";
			for(auto q:x.second)
			{
				cout<<q<<" ";
			}
			cout<<endl;
		}
		vector<string>finalSrc(chunkNo);
		sort(PieceSel.begin(),PieceSel.end(),comparator);
		for(auto x:PieceSel)
		{
			int randIdx = rand() % x.second.size();
			string SrcPrt=x.second[randIdx];
			finalSrc[x.first-1]=SrcPrt;
		}
		pthread_t pt;
		int ptr=0;
		for(auto x:finalSrc)
		{
			//srcPort......destination...filename....chunkNo.....chunksize
			string src=x+" "+client_command[3]+" "+client_command[2]+" "+to_string(ptr)+" ";
			if(ptr==chunkNo-1)
				src+=to_string(LchunkS)+" ";
			else
				src+="524288 ";
			char Cd[1024]={0};
			for(int i=0;i<src.length();i++)
			{
        		Cd[i]=src[i];
    		}
			printf("%s",Cd);
			printf("\n");
			if (pthread_create(&pt, NULL, finalFile, &Cd) < 0)
			{
				perror("\nError in thread creation\n");
			}
			pthread_join(pt,NULL);
			Files[id].chunk_present[ptr]=true;
			ptr++;
			//pthread_detach(pt);	
		}
		Downloaded[client_command[2]]=true;
		string newSHA=findSHA1(Files[id].loc);
		if(newSHA==(fileSHAs+" "+chunkSHAs))
			cout<<"File Hash and Chunk Hashs MATCHED !"<<endl;
		cout<<"FILE DOWNLOADED !"<<endl;
	}
}

string findFileName(string filePath)
{
	int len = filePath.length();
	string fname = "";
	if (filePath[len - 1] == '/')
	{
		filePath.pop_back();
		len -= 1;
	}
	size_t pos = filePath.find_last_of('/');
	for (int i = pos + 1; i < len; i++)
	{
		fname += filePath[i];
	}
	return fname;
}


void UploadFile(vector<string> client_command, string command, int sock)
{
	if (client_command.size() != 3)
	{
		printf("Invalid No. of Arguments");
		return;
	}
	//fileSHA1 will contain sha1ofCompleteFile followed by
	//sha1of-N-Chunks
	string fileSHA1 = findSHA1(client_command[1]);
	if (fileSHA1.length() == 0)
	{
		printf("Unable to open file");
		return;
	}

	string fname = findFileName(client_command[1]);
	command += " ";
	command += fname;
	command += " ";
	command += fileSHA1;
	int Cnum=GtotalSize / Chunk_Size;
	int LCsize=Chunk_Size;
	if (GtotalSize % Chunk_Size != 0)
	{
		LCsize = GtotalSize-(Cnum*Chunk_Size);
		Cnum+=1;
	}
	GtotalSize=0;
	command+=" ";
	command+=to_string(Cnum);
	command+=" ";
	command+=to_string(LCsize);
	//command contains :
	//<upload_file> <filepath> <grpID> <filename> <fileSHA> <chunkwiseSHA> <numofCHunks> <lastchunksize>
	string tracker_reply = tracker_comm(command, sock);
	if (tracker_reply == "File Uploaded.")
	{
		int len = Files.size();
		Files.push_back(cf);
		FilesInfo[fname] = len;
		Files[len].name = fname;
		Files[len].loc = client_command[1];
		Files[len].gid=client_command[2];
		Files[len].chunk_count=Cnum;
		Files[len].last_chunk=LCsize;
		Files[len].chunk_present.assign(Files[len].chunk_count,true);
		Downloaded[fname]=true;
	}
}

void ShowDownloads(int sock)
{
	for(auto x:Downloaded)
	{
		int id=FilesInfo[x.first];
		if(x.second==true)
			cout<<"[C]"<<" ["<<Files[id].gid<<"] "<<Files[id].name<<endl;
		else
			cout<<"[D]"<<" ["<<Files[id].gid<<"] "<<Files[id].name<<endl;
	}
}

int main(int argc, char const *argv[])
{
	//./client ip ip:port trackerinfo.txt
	if (argc != 3)
	{
		cout << "Invalid Number of Arguments";
		return 0;
	}

	string tinfo=argv[2];
    fstream tfile(tinfo,ios::in);
    vector<string> TINFO;
    string temp;
    while(getline(tfile,temp,' ')){
        TINFO.push_back(temp);
    }
    string ip=TINFO[0];
    int trackerPORT =stoi(TINFO[1]);
    tfile.close();

	string IP_PORT=argv[1];
	size_t _t=IP_PORT.find(":");
	int PORT = stoi(IP_PORT.substr(_t+1,(IP_PORT).length()));
	string IP = IP_PORT.substr(0,_t);
	pthread_t s;

	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	//char buffer[1024] = {0};
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	//GSock=sock;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(trackerPORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}
	cout << "connected to port: " << trackerPORT << endl;

	if (pthread_create(&s, NULL, peer_server, &PORT) < 0)
	{
		perror("\nError in thread creation\n");
	}

	string sendingP=to_string(PORT);
	send(sock,sendingP.c_str(), strlen(sendingP.c_str()), 0);
	while (1)
	{
		string command;
		vector<string> client_command;
		getline(cin, command);
		if(command.length()==0)
			continue;
		readCommand(command, client_command);
		//upload_file <file_path> <group_id >
		if (client_command[0] == "upload_file")
		{
			UploadFile(client_command, command, sock);
		}
		//download_file <group_id> <file_name> <destination_path>
		else if (client_command[0] == "download_file")
		{
			DownloadFile(client_command, command, sock);
		}
		else if(client_command[0]=="show_downloads")
		{
			ShowDownloads(sock);
		}
		else
		{
			tracker_comm(command, sock);
		}
	}
	close(sock);
	return 0;
}
