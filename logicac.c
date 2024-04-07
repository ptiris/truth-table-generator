#include <regex.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
enum{
    TK_NOTYPE=256,TK_AND,TK_OR,TK_NOT,TK_LP,TK_RP,TK_VAR,
};
enum{
    MD_OUTPUT=1,PDF_OUTPUT,PNG_OUTPUT,
};
static int FLAG_OPTIONS[32];
char DEFAULT_NAME[128];
static struct rule
{
    const char *regex;
    int token_type;
} rules[] = 
{
    {"\\(",TK_LP},
    {" +",TK_NOTYPE},
    {"\\!",TK_NOT},
    {"\\)",TK_RP},
    {"\\+",TK_OR},
    {"\\*",TK_AND},
    {"[A-Z]",TK_VAR}
};
static struct cmd
{
    const char *name;
    const char description[128];
}lsit[]={
    {"pdf","output the result in the form of pdf"},
    {"m","output the result in the form of markdown"},
    {"o","output the result files with specified name"},
    {"h","print the available options"},
    {"png","output the result in the form of png"}
};

#define NR_REGEX 7
static regex_t re[NR_REGEX]={};
void init_flags(int argc,char **argv)
{
    DEFAULT_NAME[0]='a';
    DEFAULT_NAME[1]='\0';
    for(int i=1;i<argc;i++)
    {
        //int LEN = strlen(argv[i]);
        if(!strcmp(argv[i]+1,"pdf"))
        {
            FLAG_OPTIONS[PDF_OUTPUT] = 1;
            #ifdef DEBUG
            printf("PDF is on\n");
            #endif
        }
            
        else if(!strcmp(argv[i]+1,"m"))
        {
            FLAG_OPTIONS[MD_OUTPUT] = 1;
            #ifdef DEBUG
            printf("MD is on\n");
            #endif
        }
        else if(!strcmp(argv[i]+1,"h"))
        {
            printf("usage    functions\n");
            for(int i=0;i<4;i++)
                printf(" -%s      %s\n",lsit[i].name,lsit[i].description);
            exit(0);
        }
        else if(!strcmp(argv[i]+1,"png"))
        {
            FLAG_OPTIONS[PNG_OUTPUT] = 1;
            #ifdef DEBUG
            printf("PNG is on\n");
            #endif
        }
        else if(!strcmp(argv[i]+1,"o"))
        {
            if(i+1 == argc){printf("ERROR NO FILE NAME\n");exit(-1);}
            else strcpy( DEFAULT_NAME, argv[i+1]);
        }  
    }
}
void init_regex()
{
    char err_msg[128];
    for(int i=0;i<NR_REGEX;i++)
    {
        int ret=regcomp(&re[i],rules[i].regex,REG_EXTENDED);
        if(ret)
        {
            regerror(ret,&re[i],err_msg,128);
            printf("An error occur when compiling the regex expression\n%s\n%s",err_msg,rules[i].regex);
            exit(-1);
        }
    }
}

typedef struct token
{
    int type;
    bool success;
    char str[32];
}Token;

static Token tokens[32];
static int nr_token=0;

static bool make_token(char *e)
{
    regmatch_t pmatch;
    int pos=0;
    while(e[pos]!='\0')
    {
        bool flag = 0;
        for(int i=0;i<NR_REGEX;i++)
        {
            if(regexec(&re[i],e+pos,1,&pmatch,0) == 0 && pmatch.rm_so == 0)
            {
                char *sube_start=e+pos;
                int sube_len=pmatch.rm_eo;
                printf("Match RULES %d : %s at positon %d with sube %s\n",i,rules[i].regex,pos,sube_start);
                pos+=sube_len;

                switch (rules[i].token_type)
                {
                    case TK_NOTYPE:
                        break;
                    case TK_LP:
                        nr_token++;
                        tokens[nr_token].type=TK_LP;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    case TK_RP:
                        nr_token++;
                        tokens[nr_token].type=TK_RP;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    case TK_AND:
                        nr_token++;
                        tokens[nr_token].type=TK_AND;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    case TK_OR:
                        nr_token++;
                        tokens[nr_token].type=TK_OR;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    case TK_VAR:
                        if(tokens[nr_token].type == TK_VAR){
                            nr_token++;
                            tokens[nr_token].type=TK_AND;
                            tokens[nr_token].str[0]='*';
                            tokens[nr_token].str[1]='\0';
                        }
                        nr_token++;
                        tokens[nr_token].type=TK_VAR;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    case TK_NOT:
                        nr_token++;
                        tokens[nr_token].type=TK_NOT;
                        strncpy(tokens[nr_token].str,sube_start,sube_len);
                        break;
                    default:
                        break;
                }
                flag = 1;
                break;
            }
        }
        if(flag == 0)
        {
            printf("An error occur when find no rules at %d with %s \n",pos,e+pos);
            return false;
        }
    }
    return true;
}

static int nr_vars = 0;
static bool map_var[128];
static int var_to_val[128];
static bool enum_val[32];
static bool output_val[16512];

void init_var()
{
    for(int i=1;i<=nr_token;i++)
    {
        if(tokens[i].type == TK_VAR)
            if(!map_var[(int)tokens[i].str[0]])
                map_var[(int)tokens[i].str[0]] = 1;
    }
    for(int i='A';i<='Z';i++)
        if(map_var[i])var_to_val[i]=++nr_vars;
    return;
}
int check_pars(int l,int r)
{
    int top = 0,flag = 1;
    for(int i=l;i<=r;i++)
    {
        if(tokens[i].type == TK_LP)top++;
        if(tokens[i].type == TK_RP)
        {
            if(!top)return -1;
            else top--;
        }
        if(top == 0 && i!=r)flag = 0;
    }
    return flag;
}
int pos_main(int l,int r)
{
    int top=0;
    for(int i=r;i>=l;i--)
    {
        if(tokens[i].type == TK_RP)top++;
        if(tokens[i].type == TK_LP)top--;
        if(tokens[i].type == TK_OR && top == 0)return i;
    }

    for(int i=r;i>=l;i--)
    {
        if(tokens[i].type == TK_RP)top++;
        if(tokens[i].type == TK_LP)top--;
        if(tokens[i].type == TK_NOT && top == 0)return i;
    }    

    for(int i=r;i>=l;i--)
    {
        if(tokens[i].type == TK_RP)top++;
        if(tokens[i].type == TK_LP)top--;
        if(tokens[i].type == TK_AND && top == 0)return i;
    }
    printf("Failed to find main operator in interval %d %d\n",l,r);
    exit(-1);
}
int expr(int l,int r)
{
    if(l>r){return -1;}
    if(l==r)
    {
        if(tokens[l].type != TK_VAR)return -1;
        return enum_val[var_to_val[(int)tokens[l].str[0]]];
    }
    else
    {
        int ret=check_pars(l,r),ret1,ret2;
        if(ret)return expr(l+1,r-1);
        int op = pos_main(l,r);
        #ifdef DEBUG
        printf("%d %d %d %s\n",l,r,op,tokens[op].str);
        #endif
        switch (tokens[op].type)
        {
        case TK_AND:
            ret1 = expr(l,op-1);
            ret2 = expr(op+1,r);
            #ifdef DEBUG
            printf("%d %d %d : %d %d\n",l,r,op,ret1,ret2);
            #endif
            if(ret1 == -1 || ret2 == -1)return -1;
            else return ret1 & ret2;
            break;
        case TK_OR:
            ret1 = expr(l,op-1);
            ret2 = expr(op+1,r);
            #ifdef DEBUG
            printf("%d %d %d : %d %d\n",l,r,op,ret1,ret2);
            #endif
            if(ret1 == -1 || ret2 == -1)return -1;
            else return ret1 | ret2;
        case TK_NOT:
            ret1 = expr(op+1,r);
            if(ret1 == -1)return -1;
            else return !ret1;
        default:
            break;
        }
    }
    return -1;
}
void enum_expr()
{
    for(int i=0;i<=(1<<nr_vars)-1;i++)
    {
        for(int j=1;j<=nr_vars;j++)enum_val[j]=0;
        for(int j=1;j<=nr_vars;j++)
            if(i & (1<<(j-1)))enum_val[j]=1;
        int val = expr(1,nr_token);
        output_val[i]=val;
        if(!FLAG_OPTIONS[MD_OUTPUT] && !FLAG_OPTIONS[PDF_OUTPUT])
        {
            for(int j=1;j<=nr_vars;j++)
            printf("%d ",enum_val[j]);
            printf(" %d\n",val);
        }
    }
    return;
}    

char MD_NAME [128]={};
char PDF_NAME [128]={};
char PANDOC_CMD [128]={};
char PNG_CMD [128]={};
char PNG_NAME [128]={};
char DEL_MD [128]={};
char DEL_PDF [128]={};
char OPEN [128]={};

void init_path()
{
    strcat(MD_NAME,DEFAULT_NAME);
    strcat(MD_NAME,".md");

    strcat(PDF_NAME,DEFAULT_NAME);
    strcat(PDF_NAME,".pdf");

    strcat(PNG_NAME,DEFAULT_NAME);
    strcat(PNG_NAME,".png");

    strcat(PANDOC_CMD,"pandoc -f markdown -t pdf -o ");
    strcat(PANDOC_CMD,PDF_NAME);
    strcat(PANDOC_CMD," ./");
    strcat(PANDOC_CMD,MD_NAME);

    strcat(PNG_CMD,"pdftoppm -png ");
    strcat(PNG_CMD,PDF_NAME);
    strcat(PNG_CMD," > ");
    strcat(PNG_CMD,PNG_NAME);
    
    strcat(DEL_MD,"rm ");
    strcat(DEL_PDF,"rm ");
    strcat(DEL_MD,MD_NAME);
    strcat(DEL_PDF,PDF_NAME);

    strcat(OPEN,"open ");
    if(FLAG_OPTIONS[PDF_OUTPUT])strcat(OPEN,PDF_NAME);
    else if(FLAG_OPTIONS[PNG_OUTPUT])strcat(OPEN,PNG_NAME);
}
void gen_md(char e[])
{
    FILE *fp;
    fp=fopen(MD_NAME,"w");
    fprintf(fp,"|");
    for(int i='A';i<='Z';i++)
        if(map_var[i])fprintf(fp,"%c|",(char)i);
    fprintf(fp,"%s|\n",e);
    for(int i=1;i<=nr_vars+1;i++)
        fprintf(fp,"|:--:|"+(i!=1));
    fprintf(fp,"\n");
    for(int i=0;i<=(1<<nr_vars)-1;i++)
    {
        for(int j=1;j<=nr_vars;j++)
            if(i & (1<<(j-1)))fprintf(fp,"|1|"+(j!=1));
            else fprintf(fp,"|0|"+(j!=1));
        fprintf(fp,"%d|\n",output_val[i]);
    }
    fclose(fp);
}

void gen_out(char e[])
{
    #ifdef DEBUG
    printf("%s\n",DEL_MD);
    printf("%s\n",DEL_PDF);
    #endif
    if(FLAG_OPTIONS[PDF_OUTPUT] || FLAG_OPTIONS[MD_OUTPUT] || FLAG_OPTIONS[PNG_OUTPUT])gen_md(e);
    else return;
    if(FLAG_OPTIONS[PDF_OUTPUT] || FLAG_OPTIONS[PNG_OUTPUT])system(PANDOC_CMD);
    if(FLAG_OPTIONS[PNG_OUTPUT])system(PNG_CMD);
    if(!FLAG_OPTIONS[MD_OUTPUT])system(DEL_MD);
    if(!FLAG_OPTIONS[PDF_OUTPUT])system(DEL_PDF);
    if(FLAG_OPTIONS[PDF_OUTPUT] || FLAG_OPTIONS[PNG_OUTPUT])system(OPEN);
}
int main(int argc, char **argv)
{
    init_flags(argc,argv);
    char e[128];
    fgets(e,127,stdin);
    for(int i=0;i<=(int)strlen(e);i++)
        if(e[i]=='\n')e[i]='\0';
    init_regex();
    if(!make_token(e))return -1;
    init_var();
    init_path();
#ifdef DEBUG
    for(int i=1;i<=nr_token;i++)
    {
        printf("%d %s\n",tokens[i].type,tokens[i].str);
    }
#endif
    enum_expr();
    gen_out(e);
}