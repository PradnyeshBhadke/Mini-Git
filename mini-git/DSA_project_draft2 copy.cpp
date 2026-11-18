#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
using namespace std;

// ─── STRUCTS ──────────────────────────────
struct FileSnapshot {
    string filename;
    string content;
    FileSnapshot* next;
};

struct CommitNode {
    int id;
    string message;
    string timestamp;
    string author;
    FileSnapshot* files;
    CommitNode* next;
};

struct StackNode {
    FileSnapshot data;
    StackNode* next;
};

struct StagingStack {
    StackNode* top;
    int size;
};

struct QNode {
    string message;
    QNode* next;
};

struct CommitQueue {
    QNode* front;
    QNode* rear;
    int size;
};

// ─── GLOBALS ─────────────────────────────
CommitNode* HEAD = nullptr;
CommitNode* commitListHead = nullptr;
StagingStack stagingArea;
CommitQueue msgQueue;
int commitCounter = 0;
string repoName = "";

// ─── UTILS ───────────────────────────────
string getCurrentTime() {
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

string hashContent(const string& s) {
    unsigned long h = 5381;
    for(char c : s) h = ((h << 5) + h) + (unsigned char)c;
    stringstream ss; ss << hex << h;
    return ss.str().substr(0,7);
}

void printSeparator(char c='-', int n=50) { cout << string(n,c) << "\n"; }

// ─── STACK ───────────────────────────────
void initStack(StagingStack &s)
{ 
    s.top=nullptr; s.size=0; 
}
bool isStackEmpty(StagingStack &s)
{
     return s.top==nullptr; 
    }
void push(StagingStack &s, FileSnapshot f){
    StackNode* node = new StackNode();
    node->data=f; node->next=s.top; s.top=node; s.size++;
    cout<<"[STAGED] "<<f.filename<<"  hash:"<<hashContent(f.content)<<"\n";
}
FileSnapshot pop(StagingStack &s){
    StackNode* temp = s.top;
    FileSnapshot f = temp->data;
    s.top = s.top->next; delete temp; s.size--;
    return f;
}
void displayStack(StagingStack &s){
    if(isStackEmpty(s)){ cout<<"(staging empty)\n"; return; }
    StackNode* curr=s.top; int i=1;
    while(curr){ cout<<i++<<". "<<curr->data.filename<<"\n"; curr=curr->next; }
}

// ─── QUEUE ───────────────────────────────
void initQueue(CommitQueue &q){ q.front=q.rear=nullptr; q.size=0; }
bool isQueueEmpty(CommitQueue &q){ return q.front==nullptr; }
void enqueue(CommitQueue &q, string msg){
    QNode* n=new QNode(); n->message=msg; n->next=nullptr;
    if(!q.rear) q.front=q.rear=n; else { q.rear->next=n; q.rear=n; }
    q.size++;
}
string dequeue(CommitQueue &q){
    if(isQueueEmpty(q)) return "";
    QNode* temp=q.front; string msg=temp->message;
    q.front=q.front->next; if(!q.front) q.rear=nullptr; delete temp; q.size--;
    return msg;
}

// ─── LINKED LIST COMMIT HISTORY ─────────
void appendCommit(CommitNode* c){
    if(!commitListHead) commitListHead=c;
    else { CommitNode* curr=commitListHead; while(curr->next) curr=curr->next; curr->next=c; }
    HEAD=c;
}
CommitNode* findCommitById(int id){
    CommitNode* curr=commitListHead;
    while(curr){ if(curr->id==id) return curr; curr=curr->next; }
    return nullptr;
}

// ─── REPO I/O ───────────────────────────
void initRepository(){
    printSeparator('='); cout<<"INIT REPOSITORY\n"; printSeparator('=');
    cout<<"Repo name: "; cin.ignore(); getline(cin, repoName);
    system(("mkdir \""+repoName+"\" 2>nul").c_str());
    system(("mkdir \""+repoName+"/.mygit\" 2>nul").c_str());
    system(("mkdir \""+repoName+"/.mygit/commits\" 2>nul").c_str());
    initStack(stagingArea); initQueue(msgQueue);
    commitCounter=0; HEAD=nullptr;
    cout<<"Repository '"<<repoName<<"' initialized.\n";
}

void saveCommit(CommitNode* c){
    string path = repoName + "/.mygit/commits/"+to_string(c->id)+".txt";
    ofstream out(path);
    out<<"Commit #"<<c->id<<"\nMessage: "<<c->message<<"\nAuthor: "<<c->author<<"\nTime: "<<c->timestamp<<"\nFiles:\n";
    FileSnapshot* f=c->files;
    while(f){ out<<f->filename<<" hash:"<<hashContent(f->content)<<"\n"; f=f->next; }
    out.close();
}

// ─── FEATURES ───────────────────────────
void addFile(){
    cout<<"Filename: "; string fn; cin.ignore(); getline(cin, fn);
    cout<<"File content (end with END):\n";
    string content,line; while(getline(cin,line)&&line!="END") content+=line+"\n";
    FileSnapshot f; f.filename=fn; f.content=content; f.next=nullptr;
    push(stagingArea,f);
}

void commitChanges(){
    if(isStackEmpty(stagingArea)){ cout<<"Nothing staged!\n"; return; }
    cin.ignore();
    cout<<"Commit message: "; string msg; getline(cin,msg);
    cout<<"Author: "; string author; getline(cin,author);
    enqueue(msgQueue,msg);
    string processedMsg = dequeue(msgQueue);

    FileSnapshot* head=nullptr; FileSnapshot* tail=nullptr;
    while(!isStackEmpty(stagingArea)){
        FileSnapshot f=pop(stagingArea);
        FileSnapshot* fs=new FileSnapshot(); *fs=f; fs->next=nullptr;
        if(!head) head=tail=fs; else { tail->next=fs; tail=fs; }
    }

    CommitNode* c=new CommitNode();
    c->id=++commitCounter; c->message=processedMsg; c->author=author;
    c->timestamp=getCurrentTime(); c->files=head; c->next=nullptr;
    appendCommit(c); saveCommit(c);
    cout<<"Committed #"<<c->id<<" successfully.\n";
}

void viewLog(){
    if(!commitListHead){ cout<<"No commits.\n"; return; }
    CommitNode* curr=commitListHead;
    while(curr){
        cout<<"Commit #"<<curr->id<<"  Msg:"<<curr->message<<"  Author:"<<curr->author<<"\n";
        curr=curr->next;
    }
}

void checkoutCommit(){
    cout<<"Commit ID to restore: "; int id; cin>>id;
    CommitNode* c=findCommitById(id);
    if(!c){ cout<<"Commit not found!\n"; return; }
    FileSnapshot* f=c->files;
    while(f){ ofstream out(repoName+"/"+f->filename); out<<f->content; out.close(); cout<<"Restored "<<f->filename<<"\n"; f=f->next; }
    cout<<"Working directory restored to commit #"<<id<<"\n";
}

void viewStatus(){
    cout<<"Staged files:\n"; displayStack(stagingArea);
    cout<<"HEAD commit: ";
    if(HEAD) cout<<HEAD->id<<"\n"; else cout<<"none\n";
}

// ─── MENU ───────────────────────────────
void displayMenu(){
    printSeparator('='); cout<<"Mini-Git Menu\n"; printSeparator('=');
    cout<<"1. Init Repository\n2. Add File\n3. Commit Changes\n4. View Log\n5. Checkout Commit\n6. Status\n0. Exit\nChoice: ";
}

int main(){
    int choice;
    do{
        displayMenu(); cin>>choice;
        switch(choice){
            case 1: initRepository(); break;
            case 2: addFile(); break;
            case 3: commitChanges(); break;
            case 4: viewLog(); break;
            case 5: checkoutCommit(); break;
            case 6: viewStatus(); break;
            case 0: cout<<"Goodbye!\n"; break;
            default: cout<<"Invalid option\n";
        }
    }while(choice!=0);
    return 0;
}