// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"


// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5
#define HISTORY  6
#define MAX_HISTORY 16
#define MAX_LINE_LEN 128
#define MAXARGS 10
#define NULL (0)
int histSize = 0;
typedef struct hist hist;

struct cmd {
  int type;
};

//history linked list data-structure
struct hist{
	char  histBuff[MAX_LINE_LEN];
	hist *next;	
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
	{
	  printf(2, "$ ");
	  memset(buf, 0, nbuf);
	  gets(buf, nbuf);
	  if(buf[0] == 0) // EOF
	    return -1;
	  return 0;
}

//:::::::::::::::::HISTORY FUNCTIONS STRUCT RELATED;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//the method adds the next history command link to the history linked-list data structure.
// historyLst - pointer to the head of the list, dataBuf - pointer to the command buffer.
 hist* histAppandTail(hist* historyLst, char * dataBuf){
 	if (histSize<16){ //check if the list is full
	 	histSize = histSize + 1; //increase the number of recorded commands
	 	hist* current = historyLst;
	 	hist* toAdd = (hist*)malloc(sizeof(hist)); //allocate space to the next link
	 	toAdd->next = NULL;
	 	strcpy(toAdd->histBuff, dataBuf); //copy the command buffer data to the link's buffer
	 	//first time we add to the list
	 	if (current == NULL){
	 		current = toAdd;
	 		return current;
	 	}
	 	//next additions
	 	while (current->next != NULL){ //get to the tail position
	 		current = current->next;
	 	}
	 	current->next = toAdd;
 	}
 	else{
 		//if thhe list is full, we'll decrease its size by 1 and shift the entire list,
 		//so that the second link is now the head. now we can use the method accordingly.
 		histSize = histSize - 1;
 		historyLst = histAppandTail(historyLst->next,dataBuf);
 	}


 	return historyLst;
 }

//prints the entire content of the history linked-list
 void print_history (hist* history){
 	int i = 0;
 	hist* cur = history;
 	while (cur != NULL){
 		printf(1, "%d) %s", i, cur->histBuff);
 		cur = cur->next;
 		i++;
 	}
 }

//compares a given string to a target's prefix.
int ourComp(char * target, char * toComp){
	int len_comp = strlen (toComp);
	char copy [len_comp];
	int i;
	for (i = 0 ; i< len_comp ; i++){
		copy[i] = target[i];
	}
	return strcmp(toComp,copy);
}

void activateHistoryCommand(hist* history, int index){
	int i;
	hist* cur = history;
	for (i=0; i<index; i++){
		cur = cur->next;
	}
	
	//printf(2, "debug0: %s\n", cur->histBuff);
	if (ourComp(cur->histBuff, "history")==0){
			if(ourComp (cur->histBuff,"history -l")==0){//to check that there is integer in buf[11]
				int hist_index = atoi(cur->histBuff+11);//buf+11 is the offset to the number
				if (hist_index>15){
					printf(2, "%s\n", "Error: Max index is 15!");
				}
				else{
					//printf(2, "debug1: %s\n", cur->histBuff);
					activateHistoryCommand(history, hist_index);
				}
			}
			else{
				//printf(2, "debug2: %s\n", cur->histBuff);
				print_history(history);
			}   		
		}
    	else if(fork1() == 0){
    		//printf(2, "debug3: %s ", cur->histBuff);
      		runcmd(parsecmd(cur->histBuff));
      	}
}

//;;;;;;;;;;;;;;;;;END HISTORY SECTION;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;;;;;;;;;;;;;;;;;HELPER FUNCTIONS;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
int
ourLength(const char *str)
{
    const char *s;
    for (s = str; *s; ++s);
    return (s - str)-1;
}

char * ourStrCopy(char *target, const char *source)
{
        int i;

        for(i = 0; source[i] != '\0'; ++i)
                target[i] = source[i];
        target[i] = source[i];

        return target;
}

//copies a source string from index <from> to index <to> (not including source[to])
char * boundStrCopy(char *target, const char *source, int from, int to){
	int i, j=0;
	for (i=from; i<to; i++){
		target[j] = source[i];
		j++;
	}
	target[i] = '\0';
	return target;
}
//////////////////////////set var helpers///////////////////////////
int findEqualIndex(char * buff){
	int ans;
	char* beforeEq;
	beforeEq = strchr(buff, '=');
	ans = strlen(buff) - strlen(beforeEq);
	return ans;
}

char * getVarName(char * buff){
	int i;
	int max = findEqualIndex(buff);
	char* copy = malloc(max+1);
	for (i=0; i<max; i++){
		copy[i] = buff[i];
	}
	copy[max] = '\0';
	return copy;
}



char * getVarValue(char * buff){
	int afterEqIndex = findEqualIndex(buff)+1;
	int size = strlen(strchr(buff, '='))-1;
	char* ret = malloc(size);
	boundStrCopy(ret, buff, afterEqIndex, strlen(buff)-1);
	ret[size-1] = '\0';
	return ret;
}

/////////////////////////////get var helpers////////////////////////////
int findDollarIndex(char* buff){
	int ans;
	char* beforeEq;
	beforeEq = strchr(buff, '$');
	ans = strlen(buff) - strlen(beforeEq);
	return ans;
}
//gets the variable name pointed by a '$' sign.
char * parseVarNameAfterDollar(char* buff){
	int j, i=0;
	char* varName;
	while(i < strlen(buff) && buff[i]!=' ' && buff[i]!='$' && buff[i]!='\n' && buff[i]!='\t' && buff[i]!='\r'){
		i++;
	}
	varName = malloc(i+1);
	for (j=0; j<i; j++){
		varName[j]=buff[j];
	}
	varName[i] = '\0';
	return varName;
}
//this Method fixes the buffer as it replaces variables with their values.
char * fixDollarBuffer(char* buff){
	int i=0, j=0;
	char tempBuf[100];
	char varName[128];
	char value[128];
	memset(tempBuf, 0, 100);
	//ourStrCopy(tempBuf, buff);
		//printf(1, "buff at the begining: %s\n", buff);
		//printf(1, "buff size at the begining: %d\n", strlen(buff));
	while (i<strlen(buff)){
		if (buff[i] != '$'){
			tempBuf[j] = buff[i];
			j++;
			i++;
		}
		else{
			memset(varName, 0, 128);
			memset(value, 0, 128);
			ourStrCopy(varName, parseVarNameAfterDollar(buff+i+1));
				//printf(1 ,"variable name is: %s\n", varName);
			getVariable(varName, value);
				//printf(1 ,"variable value is: %s\n", value);
			boundStrCopy(tempBuf+j, value, 0, strlen(value));
			j+=strlen(value);
			i+=strlen(varName)+1;
		}
	}
		//printf(1, "temp buf before copy: %s\n", tempBuf);
	ourStrCopy(buff, tempBuf);
		//printf(1, "buff after copy: %s\n", buff);
	//After the initial fix checks whether any Dollars were left in the buffer.
	//If so, it recursively fixes the buffer.
	if (strchr(buff, '$')!=NULL){ 
		fixDollarBuffer(buff);
	}

	return buff;
}




//;;;;;;;;;;;;;;;;;END HELPER FUNCTIONS;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

int
main(void)
{		
  static char buf[100];
  int fd;
  hist * historyLst = NULL;
  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        printf(2, "cannot cd %s\n", buf+3);
      continue;
    }

//;;;;debugging section;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	//char test1[] = "var=3";
	//char test2[] = "echo $var $x";
	//printf(1, "%d\n", preSetVariable(test1));
	//setVariable(getVarName(test1), getVarValue(test1));
	//setVariable ("x","hen");
	//printf(1 ,"before: %s\n", test2);
	//fixDollarBuffer(test2);
	//printf(1 ,"after: %s\n", test2);
	//printf(1, "%s\n", toCopy);
   	//printf(1, "%d\n", findDollarIndex(test1));
   	//printf(1, "%s\n", parseVarNameAfterDollar(test1+6));
  	//char val[128];
  	//printf (1, "%d\n" , setVariable ("x","23"));  //debbug of vars datastructure
  	//printf (1, "%d\n" , setVariable ("y","history"));
  	//printf(1,"%d\n",getVariable("x",val));
  	//printf(1,"%s\n", val);
  	//printf (1, "%d\n" , setVariable ("x","FUCKK"));
  	//printf(1,"%d\n",getVariable("var",val));
  	//printf(1, "%s\n", val);
  	//printf(1,"%s\n", val);
  	//printf(1,"%d\n",getVariable("x",val));
  	//printf(1,"%d\n",remVariable("x"));
  	//printf(1,"%d\n",getVariable("y",val));
  
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    if (strlen(buf)>MAX_LINE_LEN){
    	printf(2, "%s\n", "Error: Command is to long, more than 128 chars");
    }
	else{
		historyLst = histAppandTail(historyLst, buf);
		//printf(1, "%d\n", histSize);
		if (strchr(buf, '$')!=NULL){
			//printf(1, "before fix: %s\n", buf);
			fixDollarBuffer(buf);
			//printf(1, "real fix: %s\n", buf);
		}
		if (strchr(buf, '=')!=NULL){
				//char tempVal[128];
				//printf(1, "Found Assignment\n");
				//printf(1, "var name: %s\n", getVarName(buf));
				//printf(1, "var value: %s\n", getVarValue(buf));
			setVariable(getVarName(buf), getVarValue(buf));
				//getVariable(getVarName(buf), tempVal);
				//printf(1, "received value: %s\n", tempVal);
		}
		else if (ourComp(buf, "history")==0){
			if(ourComp (buf,"history -l")==0){//to check that there is integer in buf[11]
				int hist_index = atoi(buf+11);//buf+11 is the offset to the number
				if (hist_index>15){
					printf(2, "%s\n", "Error: Max index is 15!");
				}
				else{
					activateHistoryCommand(historyLst, hist_index);
				}
			}
			else{
			print_history(historyLst);
			}   		
		}
    	else if(fork1() == 0){
      		runcmd(parsecmd(buf));
      	} 	
    }
    wait();
  }
  exit();
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
