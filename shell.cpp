#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h> 
#include <sys/wait.h>   
#include <cstdlib>
#include <filesystem>
#include <cctype>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <readline/readline.h>
#include <readline/history.h>
using namespace std;

static int last_history_written = 0;

// Helper: Checks whether a command is a shell built-in
bool is_builtin(string &token){
  	return (token=="echo" || token=="exit" || token=="type" || token=="pwd" || token=="cd" || token=="history");
}

// Helper: Splits PATH environment variable into individual directories
vector<string> split_path(const string &path){
	vector<string> dirs;
  	string temp;
  	for(char c : path){
    	if(c==':'){
      	dirs.push_back(temp);
      	temp = "";
    	}
    	else temp+=c;
  	}
  	dirs.push_back(temp);
  	return dirs;
}

/* Helper:
		Parses user input into tokens
		Handles:
		- Single quotes
		- Double quotes
		- Backslash escaping inside and outside quotes */
vector<string> parse_input(const string& input){
  	vector<string> tokens;
  	string current;
  	bool in_single_quotes = false;
  	bool in_double_quotes = false;
  	for(size_t i=0; i<input.size(); i++){
  	  	char c=input[i];
  	  	if(in_single_quotes){
  	    	if(c=='\'')
  	    		in_single_quotes=false;
  	    	else current+=c;
  	  	}
  	  	else if(in_double_quotes){
  	    	if(c=='"'){
  	      		in_double_quotes=false;
  	    	}
  	    else if(c=='\\' && i+1<input.size() && (input[i+1]=='"' || input[i+1]=='\\' || input[i+1]=='$' || input[i+1]=='`')){
  	      	current+=input[++i];
  	    }
  	    else current+=c;
  	  	}
  	  	else{
  	    	if(c=='\''){
  	      		in_single_quotes=true;
  	    	}
  	    	else if(c=='"'){
  	      	in_double_quotes=true;
  	    	}
  	    	else if(c=='\\' && i+1<input.size()){
  	      		current+=input[++i];
  	    	}
  	    	else if(isspace(c)){
  	      		if(!current.empty()){
  	        		tokens.push_back(current);
  	        		current.clear();
  	      		}
  	    	}
  	    	else current+=c;
  	  	}
  	}
  	if(!current.empty()) tokens.push_back(current);
  	return tokens;
}

// Helper: Builtin commands allowed for completion
vector<string> builtins = {"echo", "exit", "history"};

// Helper: Longest Common Prefix
string longest_common_prefix(const vector<string>& strs){
    if (strs.empty()) return "";

    string prefix = strs[0];
    for(size_t i=1; i<strs.size(); i++){
        size_t j = 0;
        while(j<prefix.size() && j<strs[i].size() && prefix[j]==strs[i][j]){
            j++;
        }
        prefix = prefix.substr(0, j);
        if(prefix.empty()) break;
    }
    return prefix;
}

// Helper: Convert vector<string> -> char*[]
vector<char*> make_argv(vector<string>& args) {
    vector<char*> argv;
    for(auto& s : args) argv.push_back(&s[0]);
    argv.push_back(nullptr);
    return argv;
}

// Helper: Split pipeline stages
vector<vector<string>> split_pipeline(const vector<string>& tokens){
    vector<vector<string>> stages;
    vector<string> current;

    for(const auto& tok : tokens){
        if(tok == "|"){
            stages.push_back(current);
            current.clear();
        } 
		else{
            current.push_back(tok);
        }
    }
    stages.push_back(current);
    return stages;
}

// ------------------------------------------------------------
// Tab auto-completion
// ------------------------------------------------------------
int handle_tab(int, int) {
    static string last_buffer;
    static bool tab_pressed_before = false;

    string buffer = rl_line_buffer;

    // Only complete first word
    if(buffer.find(' ') != string::npos){
        cout<<"\a"<<flush;
        tab_pressed_before = false;
        return 0;
    }

    // Reset TAB state if buffer changed
    if(buffer != last_buffer){
        tab_pressed_before = false;
        last_buffer = buffer;
    }

    //---------------- BUILTINS FIRST ---------------- 
    vector<string> builtin_matches;

    for(const auto& b : builtins){
        if(b.rfind(buffer, 0)==0){
            builtin_matches.push_back(b);
        }
    }

    if(builtin_matches.size() == 1){
        string suffix = builtin_matches[0].substr(buffer.size());
        rl_insert_text(suffix.c_str());
        rl_insert_text(" ");
        rl_redisplay();
        tab_pressed_before = false;
        return 0;
    }

    //---------------- PATH EXECUTABLES ----------------
    vector<string> matches;
    char* path_env = getenv("PATH");

    if(path_env){
        vector<string> dirs = split_path(path_env);

        for(const auto& dir : dirs){
            if(!filesystem::exists(dir) || !filesystem::is_directory(dir)) continue;

            for(const auto& entry : filesystem::directory_iterator(dir)){
                if(!entry.is_regular_file()) continue;

                auto perms = entry.status().permissions();
                bool exec =
                    (perms & filesystem::perms::owner_exec) != filesystem::perms::none ||
                    (perms & filesystem::perms::group_exec) != filesystem::perms::none ||
                    (perms & filesystem::perms::others_exec) != filesystem::perms::none;

                if(!exec) continue;

                string name = entry.path().filename().string();
                if(name.rfind(buffer, 0) == 0){
                    matches.push_back(name);
                }
            }
        }
    }

    if(matches.empty()){
        cout<<"\a"<<flush;
        tab_pressed_before = false;
        return 0;
    }

    sort(matches.begin(), matches.end());
    matches.erase(unique(matches.begin(), matches.end()), matches.end());

    //---------------- UNIQUE MATCH LOGIC ----------------
    if(matches.size() == 1){
    	string full = matches[0]+" ";

    	rl_point = rl_end = 0; // reset buffer
    	rl_insert_text(full.c_str());
    	rl_redisplay();

    	tab_pressed_before = false;
    	return 0;
	}

	//---------- LCP EXTENSION ----------
    string lcp = longest_common_prefix(matches);

    if(lcp.size() > buffer.size()){
        string suffix = lcp.substr(buffer.size());
        rl_insert_text(suffix.c_str());
        rl_redisplay();
        tab_pressed_before = false;
        return 0;
    }

	//---------- DOUBLE TAB LISTING ----------
	if(!tab_pressed_before){
		cout<<"\a"<<flush;
		tab_pressed_before = true;
	} 
	else{
		cout<<"\n";
		for(size_t i=0; i<matches.size(); i++){
			cout<<matches[i];
			if(i+1<matches.size()) cout << "  ";
		}
		cout<<"\n$ ";
		cout.flush();

		// restore readline state
		rl_insert_text(buffer.c_str());
		rl_redisplay();

		tab_pressed_before = false;
	}
    return 0;
}

// ------------------------------------------------------------
// Execute pipeline builtin
// ------------------------------------------------------------
void execute_builtin(vector<string>& tokens){
    if(tokens[0] == "echo"){
        for(size_t i=1; i<tokens.size(); i++){
            cout<<tokens[i];
            if(i+1<tokens.size()) cout << " ";
        }
        cout<<endl;
    }
	else if(tokens[0] == "history"){
		if(tokens.size()==3 && tokens[1]=="-r"){
			std::ifstream file(tokens[2]);
			if(!file.is_open()) return ;
			string line;
			while(getline(file, line)){
				if(line.empty()) continue;
				add_history(line.c_str());
			}
			return;
		}
		else if(tokens.size()==3 && tokens[1]=="-w"){
			std::ofstream file(tokens[2]);
			if(!file.is_open()){
				cerr<<"History cannot write to "<<tokens[2]<<endl;
				return;
			}
			int len = history_length;
			for(int i=1; i<=len; i++){
        		HIST_ENTRY* entry = history_get(i);
        		if(entry && entry->line){
        			file<<entry->line<<endl;
        		}
    		}
			return;
		}
		else if(tokens.size()==3 && tokens[1]=="-a"){
			std::ofstream file(tokens[2], std::ios::app);
			if(!file.is_open()){
				cerr<<"History cannot write to "<<tokens[2]<<endl;
				return;
			}
			int len = history_length;
			for(int i=last_history_written+1; i<=len; i++){
        		HIST_ENTRY* entry = history_get(i);
        		if(entry && entry->line){
            		file<<entry->line<<endl;
        		}
    		}
			last_history_written = len;
			return;
		}

		int len = history_length;
		int n = len;
		if(tokens.size() == 2){
			n = stoi(tokens[1]);
			if(n<0) n=0;
		}
		int start = max(1, len-n+1);
    	for(int i=start; i<=len; i++){
        	HIST_ENTRY* entry = history_get(i);
        	if(entry && entry->line){
        		cout<<"    "<<i<<"  "<<entry->line<<endl;
        	}
    	}
	}
    else if(tokens[0] == "type"){
        if(tokens.size() < 2) return;
        if(is_builtin(tokens[1])){
            cout<<tokens[1]<<" is a shell builtin"<<endl;
        } 
		else{
            char* path = getenv("PATH");
            if(!path){
                cout<<tokens[1] << ": not found\n";
                return;
            }
            vector<string> dirs = split_path(path);
            for(auto& dir : dirs){
                string full = dir+"/"+tokens[1];
                if(access(full.c_str(), X_OK) == 0){
                    cout << tokens[1] << " is " << full << endl;
                    return;
                }
            }
            cout << tokens[1] << ": not found\n";
        }
    }
    else if(tokens[0] == "pwd"){
        cout<<filesystem::current_path().string()<<endl;
    }
}

// ------------------------------------------------------------
// Multi command (|) pipeline execution
// ------------------------------------------------------------
void execute_pipeline_multi(vector<vector<string>>& stages) {
    int n = stages.size();
    vector<pid_t> pids;

    // Create N-1 pipes
    vector<vector<int>> pipes(n-1, vector<int>(2));
    for(int i=0; i<n-1; i++){
        if(pipe(pipes[i].data()) == -1){
            perror("pipe");
            return;
        }
    }

    for(int i=0; i<n; i++){
        pid_t pid = fork();
        if(pid == 0){
            // ---------- CHILD ----------
            // stdin from previous pipe
            if(i>0){
                dup2(pipes[i-1][0], STDIN_FILENO);
            }

            // stdout to next pipe
            if(i<n-1){
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // close all pipe fds
            for(auto& p : pipes){
                close(p[0]);
                close(p[1]);
            }

            // builtin or external
            if(is_builtin(stages[i][0])){
                execute_builtin(stages[i]);
                exit(0);
            }

            auto argv = make_argv(stages[i]);
            execvp(argv[0], argv.data());
            perror("execvp");
            exit(1);
        }

        pids.push_back(pid);
    }

    // ---------- PARENT ----------
    for(auto& p : pipes){
        close(p[0]);
        close(p[1]);
    }

    for(pid_t pid : pids){
        waitpid(pid, nullptr, 0);
    }
}

// ------------------------------------------------------------
// Main Shell Loop (REPL)
// ------------------------------------------------------------
int main(){
	cout<<"[MY CUSTOM SHELL IS RUNNING]\n";

  	// Flush after every std::cout / std:cerr
	cout << std::unitbuf;
  	cerr << std::unitbuf;

	// ------------------------------------------------------------
	// Load history from HISTFILE on startup
	// ------------------------------------------------------------
	char* histfile = getenv("HISTFILE");
	if(histfile){
		std::ifstream file(histfile);
		if(file.is_open()){
			std::string line;
			while(std::getline(file, line)){
				if(line.empty()) continue;
				add_history(line.c_str());
			}
			last_history_written = history_length;
		}
	}
	

  	// Bind tab key for auto completion
  	rl_bind_key('\t', handle_tab);

	// process the input
	while(1){
		char* raw = readline("$ ");
    	if(!raw) break;  
    	string input(raw);
    	free(raw);
    	if(input.empty()) continue;
    	add_history(input.c_str());

    	// Tokenize input
    	vector<string> tokens = parse_input(input);

    	// Redirection state
    	string output_file;
    	string out_file, err_file;
    	bool redirect_out = false;
    	bool redirect_err = false;
    	bool append_out = false;
    	bool append_err = false;
		bool has_pipe = false;

    	// ------------------------------------------------------------
    	// Handle I/O Redirection
    	// ------------------------------------------------------------
		for(size_t i=0; i<tokens.size(); i++){
			if(tokens[i]==">>" || tokens[i]=="1>>"){
        		redirect_out = true;
        		append_out = true;
        		out_file = tokens[i+1];
        		tokens.erase(tokens.begin()+i, tokens.begin()+i+2);
        		i--;
    		}
    		else if(tokens[i]=="2>>"){
    			redirect_err = true;
        		append_err = true;
        		err_file = tokens[i+1];
        		tokens.erase(tokens.begin()+i, tokens.begin()+i+2);
        		i--;
      		}
      		else if(tokens[i]==">" || tokens[i]=="1>"){
        		redirect_out = true;
        		append_out = false;
        		out_file = tokens[i+1];
        		tokens.erase(tokens.begin()+i, tokens.begin()+i+2);
        		i--;
      		}
      		else if(tokens[i]=="2>"){
        		redirect_err = true;
        		append_err = false;
        		err_file = tokens[i+1];
        		tokens.erase(tokens.begin()+i, tokens.begin()+i+2);
        		i--;
      		}
			else if(tokens[i]=="|"){
				has_pipe = true;
				break;
			}
    	}
		
		// ------------------------------------------------------------
    	// Handle pipeline execution (cmd1 | cmd2)
    	// ------------------------------------------------------------
		if(has_pipe==true){
			auto stages = split_pipeline(tokens);
    		execute_pipeline_multi(stages);
    		continue;
		}

    	// ------------------------------------------------------------
    	// Apply Redirection (save original FDs)
    	// ------------------------------------------------------------
    	int saved_stdout = -1;
    	int saved_stderr = -1;

    	if(redirect_out){
      		int fd = open(out_file.c_str(), O_WRONLY | O_CREAT | (append_out ? O_APPEND : O_TRUNC), 0644);
      		if(fd<0){ 
        		perror("open"); 
        		continue; 
      		}
      		saved_stdout = dup(STDOUT_FILENO);
      		dup2(fd, STDOUT_FILENO);
      		close(fd);
    	}

    	if(redirect_err){
    		int fd = open(err_file.c_str(), O_WRONLY | O_CREAT | (append_err ? O_APPEND : O_TRUNC), 0644);
    	  	if(fd<0){ 
    	    	perror("open"); 
    	    	continue;
    	  	}
    	  	saved_stderr = dup(STDERR_FILENO);
    	  	dup2(fd, STDERR_FILENO);
    	  	close(fd);
    	}

    	// ------------------------------------------------------------
    	// Built-in Command Handling
    	// ------------------------------------------------------------
	
    	// exit: terminate shell
    	if(tokens[0]=="exit"){
			if(tokens[0] == "exit"){
				// Auto-save history to HISTFILE on exit
				char* histfile = getenv("HISTFILE");
				if(histfile){
					std::ofstream file(histfile);
					if(file.is_open()){
						int len = history_length;
						for(int i=1; i<=len; i++){
							HIST_ENTRY* entry = history_get(i);
							if(entry && entry->line){
								file<<entry->line<<endl;
							}
						}
					}
				}
				return 0;
			}
		}

    	//empty input: new input req.
    	if(tokens.empty()){
    	  	continue;
    	}

    	// echo: print arguments
    	else if(tokens[0]=="echo"){
    	  	for(size_t i=1; i<tokens.size(); i++){
    	    	cout<<tokens[i];
    	    	if(i+1<tokens.size()) cout<<" ";
    	  	}
    	  	cout<<endl;
    	}

    	// pwd: print current directory
    	else if(tokens[0]=="pwd"){
    		auto full_path = filesystem::current_path();
    	  	cout<<full_path.string()<<endl;
    	}
		
		// history: past commands
		else if(tokens[0] == "history"){
			if(tokens.size()==3 && tokens[1]=="-r"){
				std::ifstream file(tokens[2]);
				if(!file.is_open()) continue;
				string line;
				while(getline(file, line)){
					if(line.empty()) continue;
					add_history(line.c_str());
				}
				continue;
			}
			else if(tokens.size()==3 && tokens[1]=="-w"){
				std::ofstream file(tokens[2]);
				if(!file.is_open()){
					cerr<<"History cannot write to "<<tokens[2]<<endl;
					continue;
				}
				int len = history_length;
				for(int i=1; i<=len; i++){
        			HIST_ENTRY* entry = history_get(i);
        			if(entry && entry->line){
            			file<<entry->line<<endl;
        			}
    			}
				continue;
			}
			else if(tokens.size()==3 && tokens[1]=="-a"){
				std::ofstream file(tokens[2], std::ios::app);
				if(!file.is_open()){
					cerr<<"History cannot write to "<<tokens[2]<<endl;
					continue;
				}
				int len = history_length;
				for(int i=last_history_written+1; i<=len; i++){
        			HIST_ENTRY* entry = history_get(i);
        			if(entry && entry->line){
            			file<<entry->line<<endl;
        			}
    			}
				last_history_written = len;
				continue;
			}

			int len = history_length;
			int n = len;
			if(tokens.size() == 2){
				n = stoi(tokens[1]);
				if(n<0) n=0;
			}
			int start = max(1, len-n+1);
    		for(int i=start; i<=len; i++){
        		HIST_ENTRY* entry = history_get(i);
        		if(entry && entry->line){
            		cout<<"    "<<i<<"  "<<entry->line<<endl;
        		}
    		}
		}

    	// cd: change directory
    	else if(tokens[0]=="cd"){
    	  	if(tokens.size()<2){
    	    	cout << "cd: missing argument\n";
    	  	}
    	  	else{
    	    	string path = tokens[1];
    	    	if(path[0]=='~'){
    	      		char* home = getenv("HOME");
    	      		if(!home){
    	        		cout << "cd: HOME not set\n";
    	        		cout << "$ ";
    	        		continue;
    	      		}
    	      		path = home;
    	    	}
    	    	if(chdir(path.c_str()) != 0){
    	        	cout << "cd: " <<path<< ": No such file or directory\n";
    	    	}
    	  	}
    	}

    	// type: identify command
    	else if(tokens[0]=="type"){
    	  	if(is_builtin(tokens[1])) cout<<tokens[1]<<" is a shell builtin\n";
    	  	else{
    	    	char* path = getenv("PATH");
    	    	if(!path){
    	      		cout<<tokens[1]<<": not found\n";
    	    	}
    	    	else{
    	      		vector<string> dirs = split_path(path);
    	      		bool not_found = true;
    	      		for(auto &dir : dirs){
    	        		string full_path = dir+'/'+input.substr(5);
    	        		if(access(full_path.c_str(), F_OK)==0){
    	          			if(access(full_path.c_str(), X_OK)==0){
    	            			cout<<input.substr(5)<<" is "<<full_path<<endl;
    	            			not_found = false;
    	            			break;
    	          			}
    	        		}
    	      		}
    	      		if(not_found) cout<<input.substr(5)<<": not found\n";
    	    	}
    	  	}
    	}

    	// ------------------------------------------------------------
    	// External Command Execution
    	// ------------------------------------------------------------
    	else{
    	  	vector<char*> args = make_argv(tokens);
    	  	pid_t pid = fork();
    	  	if(pid==0){
    	    	if(redirect_out){
    	      		int fd = open(out_file.c_str(), O_WRONLY | O_CREAT | (append_out ? O_APPEND : O_TRUNC), 0644);
    	      		if(fd<0){ 
    	        		perror("open"); 
    	        		exit(1);
    	      		}
    	      		dup2(fd, STDOUT_FILENO);
    	      		close(fd);
    	    	}
    	    	if(redirect_err){
    	      		int fd = open(err_file.c_str(), O_WRONLY | O_CREAT | (append_err ? O_APPEND : O_TRUNC), 0644);
    	      		if(fd<0){ 
    	        		perror("open");
    	        		exit(1); 
    	      		}
    	      		dup2(fd, STDERR_FILENO);
    	      		close(fd);
    	    	}

    	    	execvp(args[0], args.data());
    	    	cout << args[0] << ": command not found\n";
    	    	exit(1);
    	  	}
    	  	else if(pid>0){
    	    	waitpid(pid, nullptr, 0);
    	  	}
    	  	else{
    	    	cout<<input<<": command not found\n";
    	  	}
		}
		// ------------------------------------------------------------
    	// Restore Original stdout and stderr
		// ------------------------------------------------------------
		if(redirect_out){
			dup2(saved_stdout, STDOUT_FILENO);
    	  	close(saved_stdout);
    	}
    	if(redirect_err){
    	  	dup2(saved_stderr, STDERR_FILENO);
    		close(saved_stderr);
		}
	}
	return 0;
}
