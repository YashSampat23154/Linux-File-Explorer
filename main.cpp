#include <iostream>
#include <libintl.h>
#include <conio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <map>
#include <filesystem>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <fstream>
#include <cstdio>


using namespace std;

// Class Declarations : 

// A class used for storing all the information that we wish to display to the user.
class File_Information{
    public: 
    int index, type;
    string user, group, permissions, last_Modified, name;
    __off_t size;
};

// A class used for storing all the entities (files and directories) present in the particular path.
class Absolute_Path_Information{
    public: 
    string absolute_Path;
    File_Information* newentity = new File_Information();
    vector<File_Information> entities;
};


class Values{
    public: 
    bool file;
    bool directory;
    string path;
};



// Fucntion declaration : 
void disableNonCanonicalMood();
void enableNonCanonicalMood();
void store_Path_Information(Absolute_Path_Information &, string);
void display_Path_Information(Absolute_Path_Information, int, int);
void addInstructionToVector(string, vector<string>&);
void normalMode(Absolute_Path_Information *);
void commandMode(Absolute_Path_Information *);
void open_Entity(string name);
void relativeToAbsolute(string &);
void copyFiles(string, string, string);
void copyDirectory(string, string, string);
Values searchEntity(string, string currentPath = "/");
void deleteDirectory(string);
void clearOutputScreen(int);


// A vector used to store the current_absolute path. Will be used to visit previously visited path.
vector<Absolute_Path_Information *> current_abspath_Stack;
int stack_index = -1;


// To store the orginal state of the terminal
struct termios original_State;


// To disable Non-canonical Mode
void disableNonCanonicalMood(){
    // Return all the changed flag to normal
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_State);
    
    // Clear the screen
    printf("\e[1;1H\e[2J");

    // Turn back cursor to normal : block which blinks
    printf ("\033[0 q");

    system("reset");
}



// To enable Non-canonical Mode
void enableNonCanonicalMood(){
    // To get the current attributes of the terminal
    tcgetattr(STDIN_FILENO, &original_State);

    struct termios flag_Manipulation = original_State;

    // ICANON : Used to accept data byte by byte
    flag_Manipulation.c_lflag &= ~(ECHO | ICANON);  

    // To update the current attributes of the terminal
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &flag_Manipulation);
}


// To store the information of a particular directory
void store_Path_Information(Absolute_Path_Information &path_Info, string path){
    // Added the absolute path.
    path_Info.absolute_Path = path;


    // Adding additional information.
    struct dirent **list_Name;
    int n = scandir(path.c_str(), &list_Name, NULL, versionsort);

    if(n<0){
        cout<<"Directory Path is invalid\n";
        return;
    }



    for(int i = 0; i<n; i++){

        struct stat entity_Info;
        string entity_Path = path+"/"+list_Name[i]->d_name;

        if(stat(entity_Path.c_str(), &entity_Info) < 0){
            cout<<"Error displaying info\n";
            return;
        }

        // Index
        path_Info.newentity->index = i+1;

        // User and Group
        struct passwd *user = getpwuid(entity_Info.st_uid);
        struct group  *group = getgrgid(entity_Info.st_gid);  

        path_Info.newentity->user = user->pw_name;
        path_Info.newentity->group = group->gr_name;


        // Permissions
        path_Info.newentity->permissions = "";
        path_Info.newentity->permissions += (S_ISDIR(entity_Info.st_mode) ? "d" : "-"); 
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IRUSR ? "r" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IWUSR ? "w" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IXUSR ? "x" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IRGRP ? "r" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IWGRP ? "w" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IXGRP ? "x" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IROTH ? "r" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IWOTH ? "w" : "-");
        path_Info.newentity->permissions += (entity_Info.st_mode & S_IXOTH ? "x" : "-");


        // Last Modified
        path_Info.newentity->last_Modified = ctime(&entity_Info.st_mtime);
        path_Info.newentity->last_Modified[path_Info.newentity->last_Modified.size()-1] = '\0';


        // Size
        path_Info.newentity->size = entity_Info.st_size;


        // Name
        // path_Info.newentity->name = "";
        path_Info.newentity->name = list_Name[i]->d_name;

        // Type of entity
        path_Info.newentity->type = (int)list_Name[i]->d_type;

        // Adding these informations to entities.
        path_Info.entities.push_back(*path_Info.newentity);
    }

    free(list_Name);
}


// To display the information in a particular directory
void display_Path_Information(Absolute_Path_Information path_Info, int start, int end){

    for(int i = start; i<end; i++){
        cout<<path_Info.entities[i].index<<"\t";
        cout<<path_Info.entities[i].user<<"\t\t";
        cout<<path_Info.entities[i].group<<"\t\t";
        cout<<path_Info.entities[i].permissions<<"\t\t";
        cout<<path_Info.entities[i].last_Modified<<"\t\t";
        cout<<path_Info.entities[i].size<<"B\t\t";
        if(path_Info.entities[i].type == 4 && path_Info.entities[i].name.compare(".") !=0 && path_Info.entities[i].name.compare("..") != 0){
            cout<<"/";
        }
        cout<<path_Info.entities[i].name<<"\t\t\n";
    }

}


// To divide the instructions into multiple strings and all them to the vector
void addInstructionToVector(string instruction, vector<string>& instructionVector){
    string temp = "";
    for(int i = 0; i<instruction.length(); i++){
        if(instruction[i] == ' '){
            instructionVector.push_back(temp);
            temp = "";
        }
        else    temp+=instruction[i];
    }
    instructionVector.push_back(temp);
    return;
}


// Normal mode
void normalMode(Absolute_Path_Information *newpath){

    enableNonCanonicalMood();
    printf("\033[2J\033[1;1H");


    struct winsize window_Size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_Size);
    int init_rows = window_Size.ws_row;

    // start stores the starting index of the screen 
    // end store the number of all files and directories.
    int start = 0, end;

    if(window_Size.ws_row - 6 > 0)
        end = min((int)newpath->entities.size(), window_Size.ws_row - 6);
    else if (window_Size.ws_row - 4 <= 0){
        cout<<"Terminal Size is too Small. Please resize the terminal"<<endl;
        end = 0;
    }
    else
        end = min((int)newpath->entities.size(), window_Size.ws_row - 4);

    display_Path_Information(*newpath, start, end);

    char ch1, ch2, ch3;
    int i  = 0;
    printf("\033[%dH", i++);


    // Initially printing details of path and mode.
    if(window_Size.ws_row - 4 > 0){

        // Printing details at the end of the page
        printf("\033[%dH", window_Size.ws_row-2);

        cout<<"PATH : "<<newpath->absolute_Path<<endl;
        cout<<"-----------------NORMAL MODE-----------------";

        printf("\033[%dH", i);
    }


    while(1){

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_Size);
    
        bool changedSize = 0;
        
        // Logic to detect a change in the size of the terminal and to adjust accordingly.
        if(window_Size.ws_row != init_rows){

            bool refreshEntries = 1;

            if(window_Size.ws_row - 6 > 0)
                end = min((int)newpath->entities.size(), window_Size.ws_row - 6);
            else if (window_Size.ws_row - 4 <= 0){
                printf("\033[2J\033[1;1H");
                cout<<"Terminal Size is too Small. Please resize the terminal"<<endl;
                end = 0;
                refreshEntries = 0;
            }
            else
                end = min((int)newpath->entities.size(), window_Size.ws_row - 4);


            if(refreshEntries){
                start = 0;
                i = 1;
                printf("\033[2J\033[1;1H");
                display_Path_Information(*newpath, start, end);
                printf("\033[%dH", i);
                changedSize = 1;
            }

            init_rows = window_Size.ws_row;
        }

        if(changedSize == 1 && window_Size.ws_row - 4 > 0){
            // Printing details at the end of the page
            printf("\033[%dH", window_Size.ws_row-2);

            cout<<"PATH : "<<newpath->absolute_Path<<endl;
            cout<<"-----------------NORMAL MODE-----------------";

            printf("\033[%dH", i);
        }


        if(window_Size.ws_row - 4 > 0 && kbhit()){
            
            ch1 = getchar();

            if(ch1 == 10)
                ch1=13;

            // Checking if we have a esc character as input
            if(ch1 == '\033'){

                ch2 = getchar();
                ch3 = getchar();


                // If user enters up arrow
                if(ch2 == '[' && ch3 == 'A' && i>0){
                    if(i!=1)
                        printf("\033[%dH", --i);
                    else if(start!=0){
                        start--;
                        end--;
                        printf("\033[2J\033[1;1H"); 
                        display_Path_Information(*newpath, start, end);
                        i = 1;
                        // printf("\033[%dH", i);

                        // Printing details at the end of the page
                        printf("\033[%dH", window_Size.ws_row-2);

                        cout<<"PATH : "<<newpath->absolute_Path<<endl;
                        cout<<"-----------------NORMAL MODE-----------------";

                        printf("\033[%dH", i);
                        
                    } 
                }
                    

                // If user enters down arrow
                else if(ch2 == '[' && ch3 == 'B' && i<=end-start){
                    if(i == end-start && end != newpath->entities.size()){
                        start++;
                        end++;
                        printf("\033[2J\033[1;1H"); 
                        display_Path_Information(*newpath, start, end);
                        i = end-start;
                        // printf("\033[%dH", i);

                        // Printing details at the end of the page
                        printf("\033[%dH", window_Size.ws_row-2);

                        cout<<"PATH : "<<newpath->absolute_Path<<endl;
                        cout<<"-----------------NORMAL MODE-----------------";

                        printf("\033[%dH", i);

                    }
                    else if(i!=end-start){
                        printf("\033[%dH", ++i);
                    }   
                }
                
                // If user enters right arrow
                else if(ch2 == '[' && ch3 == 'C' && stack_index < current_abspath_Stack.size()-1){
                    stack_index++;
                    current_abspath_Stack[stack_index]->entities.clear();
                    store_Path_Information(*current_abspath_Stack[stack_index], current_abspath_Stack[stack_index]->absolute_Path);
                    normalMode(current_abspath_Stack[stack_index]);
                }

                // If user enters left arrow
                else if(ch2 == '[' && ch3 == 'D' && stack_index > 0){
                    stack_index--;
                    current_abspath_Stack[stack_index]->entities.clear();
                    store_Path_Information(*current_abspath_Stack[stack_index], current_abspath_Stack[stack_index]->absolute_Path);
                    normalMode(current_abspath_Stack[stack_index]);
                }

                // If user enters home
                else if(ch2 == '[' && ch3 == 'H'){

                    // If global vector index is not last then clear it. This is done so to replecate what linux file system does.
                    if(stack_index != current_abspath_Stack.size()-1){
                        for(int start = stack_index+1; start < current_abspath_Stack.size(); start++){
                            free(current_abspath_Stack[start]);
                        }
                        current_abspath_Stack.erase(current_abspath_Stack.begin() + stack_index+1, current_abspath_Stack.end());
                    }

                    // Adding new directory to the global vector.
                    Absolute_Path_Information *newpath1 = new Absolute_Path_Information;
                    string absolute_path = "/home/";

                    struct dirent **list_Name;
                    scandir(absolute_path.c_str(), &list_Name, NULL, versionsort);
                    absolute_path+=list_Name[2]->d_name;

                    store_Path_Information(*newpath1, absolute_path);
                    current_abspath_Stack.push_back(newpath1);
                    stack_index++;
                    normalMode(newpath1);
                }

            }  

            // Checking if we have a enter 
            else if(ch1 == 13){

                // Open the file irrespective of its type
                if(newpath->entities[start+i-1].type == 8){
                    if(!fork()){
                        execlp("xdg-open", "xdg-open", (newpath->absolute_Path+"/"+newpath->entities[start+i-1].name).c_str(), NULL);
                    }
                }

                // Go to the previous directory.
                else if(newpath->entities[start+i-1].name == ".."){

                    // If global vector index is not last then clear it. This is done so to replecate what linux file system does.
                    if(stack_index != current_abspath_Stack.size()-1){
                        for(int start = stack_index+1; start < current_abspath_Stack.size(); start++){
                            free(current_abspath_Stack[start]);
                        }
                        current_abspath_Stack.erase(current_abspath_Stack.begin() + stack_index+1, current_abspath_Stack.end());
                    }
                        
                    // Obtaining the last directory.
                    int index_Of_String = newpath->absolute_Path.length()-1;
                    while(index_Of_String>0 && newpath->absolute_Path[index_Of_String] != '/')
                        index_Of_String--;
                    string absolute_path = newpath->absolute_Path.substr(0, index_Of_String);

                    if(absolute_path.length() == 0) absolute_path = "/";

                    // Adding new directory to the global vector.
                    Absolute_Path_Information *newpath1 = new Absolute_Path_Information;
                    store_Path_Information(*newpath1, absolute_path);
                    current_abspath_Stack.push_back(newpath1);
                    stack_index++;
                    normalMode(newpath1); 

                }

                // Open directory
                else if(newpath->entities[start+i-1].name != "." && newpath->entities[start+i-1].type == 4){
                    
                    // If global vector index is not last then clear it. This is done so to replecate what linux file system does.
                    if(stack_index != current_abspath_Stack.size()-1){
                        for(int start = stack_index+1; start < current_abspath_Stack.size(); start++){
                            free(current_abspath_Stack[start]);
                        }
                        current_abspath_Stack.erase(current_abspath_Stack.begin() + stack_index+1, current_abspath_Stack.end());
                    }

                    // Adding new directory to the global vector.
                    Absolute_Path_Information *newpath1 = new Absolute_Path_Information;
                    string absolute_path = newpath->absolute_Path + "/" + newpath->entities[start+i-1].name;
                    store_Path_Information(*newpath1, absolute_path);
                    current_abspath_Stack.push_back(newpath1);
                    stack_index++;
                    normalMode(newpath1);                
                }

            }

            // We have backspace
            else if (ch1 == 127){
                if(stack_index != current_abspath_Stack.size()-1){
                    for(int start = stack_index+1; start < current_abspath_Stack.size(); start++){
                        free(current_abspath_Stack[start]);
                    }
                    current_abspath_Stack.erase(current_abspath_Stack.begin() + stack_index+1, current_abspath_Stack.end());
                }
                    
                int index_Of_String = newpath->absolute_Path.length()-1;
                while(index_Of_String>0 && newpath->absolute_Path[index_Of_String] != '/')
                    index_Of_String--;
                string absolute_path = newpath->absolute_Path.substr(0, index_Of_String);

                if(absolute_path.length() == 0) absolute_path = "/";

                Absolute_Path_Information *newpath1 = new Absolute_Path_Information;
                store_Path_Information(*newpath1, absolute_path);
                current_abspath_Stack.push_back(newpath1);
                stack_index++;
                normalMode(newpath1); 
            }

            // Switch to command mode
            else if (ch1 == ':'){
                // disableNonCanonicalMood();
                commandMode(newpath);
            }

            // User wants to exit
            else if(ch1 == 'q'){
                printf("\033[2J\033[1;1H");
                disableNonCanonicalMood();
                exit(1);
            }
            
        }

    }
    
}


// Command mode
void commandMode(Absolute_Path_Information *newpath){

    struct winsize window_Size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_Size);
    
    clearOutputScreen(window_Size.ws_row-4);
    clearOutputScreen(window_Size.ws_row-3);
    clearOutputScreen(window_Size.ws_row-2);
    clearOutputScreen(window_Size.ws_row-1);
    printf("\033[%dH", window_Size.ws_row-2);
    cout<<"PATH : "<<newpath->absolute_Path<<endl;
    cout<<"-----------------COMMAND MODE-----------------";

    int start = 0, end;

    if(window_Size.ws_row - 6 > 0)
        end = min((int)newpath->entities.size(), window_Size.ws_row - 6);
    else if (window_Size.ws_row - 4 <= 0){
        cout<<"Terminal Size is too Small. Please resize the terminal"<<endl;
        end = 0;
    }
    else
        end = min((int)newpath->entities.size(), window_Size.ws_row - 4);


    while(1){
        for(int i = 1; i<window_Size.ws_row-4; i++){
            clearOutputScreen(i);
        }
        printf("\033[%dH", 1);
        
        newpath->entities.clear();
        store_Path_Information(*newpath, newpath->absolute_Path);
        display_Path_Information(*newpath, start, end);

        printf("\033[%dH", window_Size.ws_row-4);
        cout<<"Enter the command : ";

        // char *instruction = new char[1000];
        string instruction="";


        char ch;
        while(1){

            ch = getchar();
            
            // User enters a ESC
            if(ch == 27){
                for(int i = 1; i<window_Size.ws_row; i++){
                    clearOutputScreen(i);
                }
                newpath->entities.clear();
                store_Path_Information(*newpath, newpath->absolute_Path);
                normalMode(newpath);
            } 
            // User enters a backspace
            else if(ch == 127){
                if(instruction.length() > 0)
                    instruction.pop_back();
            }
            // User enters enter
            else if(ch == 10 || ch == 13){                
                clearOutputScreen(window_Size.ws_row-4);
                printf("\033[%dH", window_Size.ws_row-4);
                cout<<"Enter the command : ";
                break;
            } 
            // User enters anything else           
            else if (ch != '\t'){
                instruction.push_back(ch);
            }
            clearOutputScreen(window_Size.ws_row-4);
            printf("\033[%dH", window_Size.ws_row-4);
            cout<<"Enter the command : ";
            cout<<instruction;                        
        }

        while(instruction[0] == ' '){
            instruction  = instruction.substr(1, instruction.length()-1);
        }


        for(int i = 0; i<instruction.length(); i++)
            cout<<instruction[i];


        vector<string> instructionVector;

        addInstructionToVector(instruction, instructionVector);

        if(instructionVector[0].compare("quit") == 0){
            printf("\033[2J\033[1;1H");
            disableNonCanonicalMood();
            exit(1);
        }
        else if (instructionVector[0].compare("goto") == 0) {
            printf("\033[2J\033[1;1H");
            
            string directory_Path = instructionVector[1];

            struct dirent **list_Name;
            int n = scandir(directory_Path.c_str(), &list_Name, NULL, versionsort);

            if(n<0){
                cout<<"Directory Path is invalid\n";
                return;
            }

            Absolute_Path_Information *newpath1 = new Absolute_Path_Information;
            store_Path_Information(*newpath1, directory_Path);
            current_abspath_Stack.push_back(newpath1);
            stack_index++;
            normalMode(newpath1);
        }
        else if (instructionVector[0].compare("copy") == 0){

            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);
            
            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/'  && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            } 
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(temp != "" && !(destExist.file == 0 && destExist.directory == 1)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"Destination does not exist";
                continue;
            }

            for(int i = 1; i<instructionVector.size()-1; i++){
                Values entityExist = searchEntity(instructionVector[i], newpath->absolute_Path);
                // relativeToAbsolute(entityExist.path);

                if(entityExist.file)
                    copyFiles(entityExist.path, destExist.path+"/"+temp, instructionVector[i]);
                else if(entityExist.directory){
                    // Checking if a folder of that name already exist inside the destination
                    Values entityInsideDestination = searchEntity(instructionVector[i], destExist.path+"/"+temp);

                    if(entityInsideDestination.directory == 1){
                        clearOutputScreen(window_Size.ws_row-3);
                        printf("\033[%dH", window_Size.ws_row-3);
                        cout<<"Entity already exist inside destination";
                        continue;
                    }

                    if(temp != ""){
                        mkdir((destExist.path+"/"+temp+"/"+instructionVector[i]).c_str(), 0777);
                        copyDirectory(entityExist.path+"/"+instructionVector[i], destExist.path+"/"+temp+"/"+instructionVector[i], instructionVector[i]);
                    }
                    else{
                        mkdir((destExist.path+"/"+instructionVector[i]).c_str(), 0777);
                        copyDirectory(entityExist.path+"/"+instructionVector[i], destExist.path+"/"+instructionVector[i], instructionVector[i]);
                    }
                }
                else{
                    clearOutputScreen(window_Size.ws_row-3);
                    printf("\033[%dH", window_Size.ws_row-3);
                    cout<<"Entity does not exist";
                }
            }
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"Entity has been copied ";

        }
        else if (instructionVector[0].compare("move") == 0){

            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);
            
            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/' && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            }
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(temp != "" && !(destExist.file == 0 && destExist.directory == 1)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"Destination does not exist";
                continue;
            }
            
            for(int i = 1; i<instructionVector.size()-1; i++){
                Values entityExist = searchEntity(instructionVector[i], newpath->absolute_Path);
                relativeToAbsolute(entityExist.path);

                if(!(entityExist.file || entityExist.directory)){
                    clearOutputScreen(window_Size.ws_row-3);
                    printf("\033[%dH", window_Size.ws_row-3);
                    cout<<"Entity isn't a file or directory";
                    continue;
                }

                // string path  = entityExist.path;
                if(temp != ""){
                    if(rename((entityExist.path+"/"+instructionVector[i]).c_str(), (destExist.path+"/"+temp+"/"+instructionVector[i]).c_str())){
                        clearOutputScreen(window_Size.ws_row-3);
                        printf("\033[%dH", window_Size.ws_row-3);
                        cout<<"Error moving";
                    }
                    else{
                        clearOutputScreen(window_Size.ws_row-3);
                        printf("\033[%dH", window_Size.ws_row-3);
                        cout<<"Moving Successful";
                    }
                }
                else{
                    if(rename((entityExist.path+"/"+instructionVector[i]).c_str(), (destExist.path+"/"+instructionVector[i]).c_str())){
                        clearOutputScreen(window_Size.ws_row-3);
                        printf("\033[%dH", window_Size.ws_row-3);
                        cout<<"Error moving";
                    }
                    else{
                        clearOutputScreen(window_Size.ws_row-3);
                        printf("\033[%dH", window_Size.ws_row-3);
                        cout<<"Moving Successful";
                    }
                }
                
                
            }
            
        }
        else if (instructionVector[0].compare("create_file") == 0){
            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);
            
            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/' && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            } 
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(temp != "" && !(destExist.file == 0 && destExist.directory == 1)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"Destination does not exist";
                continue;
            }

            string fileToBeCreated;

            if(temp != "")
                fileToBeCreated = destExist.path+"/"+temp+"/"+instructionVector[1];
            else 
                fileToBeCreated = destExist.path+"/"+instructionVector[1];
            
            ofstream {fileToBeCreated};
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"File Created";

        }
        else if (instructionVector[0].compare("create_dir") == 0){

            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);
            
            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/' && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            } 
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(temp != "" && !(destExist.file == 0 && destExist.directory == 1)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"Destination does not exist";
                continue;
            }
            
            if(temp != "")
                mkdir((destExist.path+"/"+temp+"/"+instructionVector[1]).c_str(), 0777);
            else 
                mkdir((destExist.path+"/"+instructionVector[1]).c_str(), 0777);
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"Folder Created";

        }
        else if (instructionVector[0].compare("rename") == 0){
            Values entityExist = searchEntity(instructionVector[1], newpath->absolute_Path);

            if(entityExist.file || entityExist.directory){
                string path  = entityExist.path;
                if(rename((path+"/"+instructionVector[1]).c_str(), (path+"/"+instructionVector[2]).c_str())){
                    clearOutputScreen(window_Size.ws_row-3);
                    printf("\033[%dH", window_Size.ws_row-3);
                    cout<<"Error Renaming";
                }
                else{
                    clearOutputScreen(window_Size.ws_row-3);
                    printf("\033[%dH", window_Size.ws_row-3);
                    cout<<"Renaming Successful";
                }
            }
        }
        else if (instructionVector[0].compare("search") == 0){
            Values exist;
            exist = searchEntity(instructionVector[1], newpath->absolute_Path);
            
            if(exist.file || exist.directory){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"ENTITY EXISTS";
            }
            else{   
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"ENTITY DOES NOT EXISTS";
            } 
        }
        else if (instructionVector[0].compare("delete_file") == 0){
            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);

            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/' && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            } 
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(!(destExist.file == 1 && destExist.directory == 0)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"File does not exist";
                continue;
            }

            string fileToBeDeleted;

            if(temp != "")
                fileToBeDeleted = destExist.path+"/"+temp;
            else 
                fileToBeDeleted = destExist.path+"/";
            
            remove(fileToBeDeleted.c_str());
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"File Deleted";

        }
        else if (instructionVector[0].compare("delete_dir") == 0){
            string dest_path = instructionVector[instructionVector.size()-1];
            relativeToAbsolute(dest_path);

            // Obtaining the last directory.
            string temp = "";
            int index_Of_String = dest_path.length()-1;
            while(index_Of_String>=0 && dest_path[index_Of_String] != '/' && dest_path[index_Of_String] != '~'){
                temp = dest_path[index_Of_String]+temp;
                index_Of_String--;
            } 
            dest_path = dest_path.substr(0, index_Of_String);
            if(dest_path.length() == 0) dest_path = "/";

            Values destExist = searchEntity(temp, dest_path);

            if(!(destExist.file == 0 && destExist.directory == 1)){
                clearOutputScreen(window_Size.ws_row-3);
                printf("\033[%dH", window_Size.ws_row-3);
                cout<<"File does not exist";
                continue;
            }

            string folderToBeDeleted;

            if(temp != "")
                folderToBeDeleted = destExist.path+"/"+temp;
            else 
                folderToBeDeleted = destExist.path+"/";
            
            deleteDirectory(folderToBeDeleted);
            rmdir(folderToBeDeleted.c_str());
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"Folder Deleted";
        }
        else{
            clearOutputScreen(window_Size.ws_row-3);
            printf("\033[%dH", window_Size.ws_row-3);
            cout<<"Command not found";
        }
    }

}


// To get absolute path from relative path
void relativeToAbsolute(string &dest_path){
    if(dest_path[0] == '.' && dest_path[1] == '/'){
        dest_path = get_current_dir_name() + dest_path.substr(1, dest_path.length() - 1);
    }
    else if(dest_path[0] == '.'){
        dest_path = get_current_dir_name() + dest_path.substr(1, dest_path.length() - 1) + "/";
    }
    else if (dest_path[0] == '.' &&  dest_path[1] == '.' && dest_path[2] == '/'){
        dest_path = get_current_dir_name() + dest_path.substr(2, dest_path.length() - 2);
    } 
}


// To copy files
void copyFiles(string source_path, string dest_path, string fileName){

    struct winsize window_Size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_Size);

    source_path += "/" + fileName;
    dest_path += "/" + fileName;
    
    ifstream source_entity{source_path};
    ofstream dest_entity{dest_path};

    string content;
    while(getline(source_entity, content)){
        dest_entity << content << endl;
    }

    source_entity.close();
    dest_entity.close();

}


// To copy directories
void copyDirectory(string source_path, string dest_path, string fileName){

    struct dirent **list_Name;
    int n = scandir(source_path.c_str(), &list_Name, NULL, versionsort);

    for(int i = 2; i<n ; i++){
        string name = list_Name[i]->d_name;
        if(list_Name[i]->d_type == 8)
            copyFiles(source_path, dest_path, list_Name[i]->d_name);
        else if (list_Name[i]->d_type == 4 && name.compare(fileName) != 0){
            mkdir((dest_path+"/"+list_Name[i]->d_name).c_str(), 0777);
            copyDirectory(source_path+"/"+list_Name[i]->d_name, dest_path+"/"+list_Name[i]->d_name, fileName);
        }
    }
    
}


// To search of an entity
Values searchEntity(string entityName, string currentPath){

    struct dirent **list_Name;
    int n = scandir(currentPath.c_str(), &list_Name, NULL, versionsort);
    
    Values ans;
    ans.file = 0;
    ans.directory = 0;
    ans.path = currentPath;


    if(n<0){
        ans.file = 0;
        ans.directory = 0;
        ans.path = currentPath;
        return ans;
    }

    for(int i = 2; i<n; i++){

        string check = list_Name[i]->d_name;

        ans.file = 0;
        ans.directory = 0;
        ans.path = currentPath;
        
        // Check if file or directory to see its the same as the one we need to find.
        if(list_Name[i]->d_type == 8 || list_Name[i]->d_type == 4){
    
            if( check.compare(entityName) == 0 ){

                if(list_Name[i]->d_type == 8)
                    ans.file = 1;
                else
                    ans.directory = 1;
                
                ans.path = currentPath;
                break;
            }
        }

        // If directory then search inside it.
        if(list_Name[i]->d_type == 4 && check.compare(".") !=0 && check.compare("..") != 0){
            string newPath = currentPath;
            if(newPath[newPath.length()-1] != '/')
                newPath+="/";
            newPath += check;
            ans = searchEntity(entityName, newPath);
            if(ans.file || ans.directory)
                break;
        }
        
    }

    return ans;

}


// To delete a directory
void deleteDirectory(string folderToBeDeleted){

    struct dirent **list_Name;
    int n = scandir(folderToBeDeleted.c_str(), &list_Name, NULL, versionsort);

    for(int i = 2; i<n ; i++){
        string name = list_Name[i]->d_name;
        if(list_Name[i]->d_type == 8)
            remove((folderToBeDeleted+"/"+name).c_str());
        else if (list_Name[i]->d_type == 4){
            deleteDirectory(folderToBeDeleted+"/"+name);
            rmdir((folderToBeDeleted+"/"+name).c_str());
        }
    }
}


// To clear the command line screen.
void clearOutputScreen(int row){

    struct winsize window_Size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_Size);
    int init_rows = window_Size.ws_row;

    printf("\033[%dH", row);
    for(int i = 0; i<window_Size.ws_col; i++){
        cout << " " ;
    }

}


int main()
{ 
    printf ("\033[3 q");
    enableNonCanonicalMood();  

    string directory_Path;
    directory_Path = get_current_dir_name();

    Absolute_Path_Information *newpath = new Absolute_Path_Information;
    store_Path_Information(*newpath, directory_Path);
    current_abspath_Stack.push_back(newpath);
    stack_index++;
    normalMode(newpath);


    return 0;
}