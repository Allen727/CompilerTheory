#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <memory.h>
#include <stack>
#include <assert.h>
#include <iostream>

#define Gfile "/home/yc/Desktop/编译原理/RDP/G.txt"
#define Tfile "/home/yc/Desktop/编译原理/RDP/T.cpp"

class grammer{
public:
    int gid;
    char left;
    std::string right;
    grammer(const int, const char, const std::string&);
};

const char DS = '#';
const int mxv = 100;
const int mxg = 100;

std::set<char> VN, VT, V;

// 字符与数字间的映射
std::map<char, int> charToId;
char idToChar[mxv];

int Vcnt = 0;
std::map<char, std::vector<int>> Gright;
std::vector<grammer> Gs;
char S = '\0';
int gcnt = 0;

void getRight(const std::string &);
void readGrammers();

const char epsilon = 'e';

// 产生式能否使左部为空
int visg[mxg];    // -2: derivating, -1: unknown, 0: false, 1: true
// 非终结符能符为空
int EPSILON[mxv];    // -1: unknown, 0: false, 1: true
// 下标从1开始，非终结符映射到数值，对应vector为对应的FIRST集和FOLLOW集
std::set<char> FIRST[mxv];
std::set<char> FOLLOW[mxv];

void initEPSILON(char);

bool derivateEPSILON(char);

void initEPSILON(char);

void derivateFIRST(char);

// FOLLOW集间的依赖关系，用于生成拓扑排序
bool dep[mxv][mxv] = {0};
// 入度
std::map<char,int> in_cnt;

void initFOLLOW(char);

void derivateFOLLOW();

int table[mxv][mxv];

void derivateSELECT();

grammer::grammer(const int gid, const char left, const std::string& right):gid(gid),left(left),right(right){}

void getRight(const std::string &gram){
    int s;
    char left;
    for(s=0;s<gram.length()&&gram[s]==' ';++s);
    left = gram[s];
    if(S=='\0') S=left;    // 起始非终结符
    for(s=1;s<gram.length()&&gram[s]==' ';++s);
    assert(gram[s++]=='-');
    assert(gram[s++]=='>');
    for(int i=s;i<gram.length();++i){
        V.insert(gram[i]);    // 右部所有符号
    }
    if(!VN.count(left)){
        VN.insert(left);    // 非终结符
        charToId[left]=Vcnt;
        idToChar[Vcnt]=left;
        ++Vcnt;
    }
    Gs.emplace_back(gcnt,left,gram.substr(s,gram.length()-s));
    Gright[left].push_back(gcnt);
    ++gcnt;
}

void readGrammers(){
    VT.insert(DS);    // 终结符'#'
    charToId[DS]=Vcnt;
    idToChar[Vcnt]=DS;
    ++Vcnt;
    
    std::ifstream fin(Gfile,std::ios::in);
    std::string gram;
    if(fin.is_open()){
        while(getline(fin,gram)){
            getRight(gram);
        }
        for(auto c : V){
            if(!VN.count(c)&&!VT.count(c)){
                VT.insert(c);    // 终结符
                charToId[c]=Vcnt;
                idToChar[Vcnt]=c;
                ++Vcnt;
            }
        }
    }
}

void initEPSILON(char s){
    memset(visg, -1, sizeof(visg));
    memset(EPSILON, -1, sizeof(EPSILON));
    for(auto & c: VN){
        if(EPSILON[charToId[c]]) derivateEPSILON(c);
    }
}

bool derivateEPSILON(char c){
    if(!VN.count(c)){
        if(c==epsilon) return true;
        else return false;
    }
    if(~EPSILON[charToId[c]]) return EPSILON[charToId[c]];
    for(auto & gid : Gright[c]){
        if(visg[gid]==-2) continue;    // 避免A->BAC
        if(visg[gid]==1) return EPSILON[charToId[c]] = 1;
        if(visg[gid]==0) continue;
        visg[gid] = -2;
        bool canbe = true;
        for(auto & rc : Gs[gid].right){
            if(!derivateEPSILON(rc)){    // 推导式右部存在终结符或非空非终结符
                canbe = false;
                break;
            }
        }
        visg[gid] = canbe;
        if(canbe){
            return EPSILON[charToId[c]] = 1;
        }
    }
    return EPSILON[charToId[c]] = 0;
}

void initFIRST(char s){
    for(auto & c: VN){
        if(FIRST[c].size()==0) derivateFIRST(c);
    }
}

void derivateFIRST(char c){
    for(auto & gid : Gright[c]){
        for(auto & rc : Gs[gid].right){
            if(rc==c){
                if(EPSILON[charToId[c]]==1) continue;
                else break;
            }
            if(rc==epsilon) continue;
            if(!VN.count(rc)){    // 为终结符
                FIRST[charToId[c]].insert(rc);
                break;
            }else{    // 为非终结符
                derivateFIRST(rc);
                for(auto & nc : FIRST[charToId[rc]]){
                    FIRST[charToId[c]].insert(nc);
                }
                if(!EPSILON[charToId[rc]]) break;
            }
        }
    }
}

void initFOLLOW(char s){
    FOLLOW[charToId[s]].insert(DS);
    for(auto & c : VN){
        in_cnt[c] = 0;
    }
    for(auto &g : Gs){
        for(int i=0;i<g.right.length();++i){
            if(!VN.count(g.right[i])) continue;    // 是终结符
            for(int j=i+1;j<g.right.length();++j){
                if(!VN.count(g.right[j])){    // 是终结符
                    FOLLOW[charToId[g.right[i]]].insert(g.right[j]);
                    break;
                }else{    // 是非终结符
                    for(auto & fc: FIRST[charToId[g.right[j]]]){
                        FOLLOW[charToId[g.right[i]]].insert(fc);
                    }
                    if(EPSILON[charToId[g.right[j]]]==0) break;
                }
            }
        }
        for(int i=(int)g.right.length()-1;i>=0;--i){
            if(!VN.count(g.right[i])) break;    // 是终结符
            if(g.left==g.right[i]) continue;    // 依赖自身
            if(!dep[charToId[g.left]][charToId[g.right[i]]]){
                dep[charToId[g.left]][charToId[g.right[i]]] = true;
                ++in_cnt[g.right[i]];   
            }
            if(EPSILON[charToId[g.right[i]]]==0) break;
        }
    }
    derivateFOLLOW();
}

void derivateFOLLOW(){
    std::queue<char> q;
    for(auto &e : in_cnt){
        if(e.second==0) q.push(e.first);
    }
    char A;
    while(!q.empty()){
        A = q.front();
        q.pop();
        for(int to=0;to<Vcnt;++to){
            if(dep[charToId[A]][to]){
                dep[charToId[A]][to] = false;
                --in_cnt[idToChar[to]];
                if(in_cnt[idToChar[to]]==0) q.push(idToChar[to]);
                
                for(auto & fc : FOLLOW[charToId[A]]){
                    FOLLOW[to].insert(fc);
                }
            }
        }
    }
}

void derivateSELECT(){
    memset(table, -1, sizeof(table));
    for(auto &g : Gs){
        for(auto &c: g.right){    // 为终结符
            if(!VN.count(c)){
                assert(table[charToId[g.left]][charToId[c]]==-1);
                table[charToId[g.left]][charToId[c]] = g.gid;
                break;
            }
            for(auto & fc : FIRST[charToId[c]]){    // 为非终结符
                assert(table[charToId[g.left]][charToId[fc]]==-1);
                table[charToId[g.left]][charToId[fc]] = g.gid;
            }
            if(!EPSILON[charToId[c]]) break;    // 非空非终结符
        }
        if(EPSILON[charToId[g.left]]==0) continue;
        for(auto & fc : FOLLOW[charToId[g.left]]){
            table[charToId[g.left]][charToId[fc]] = g.gid;
        }
    }
}

void printTable(){
    using std::cout;
    using std::endl;
    cout << "     ";
    for(int i=0;i<Vcnt;++i){
        cout << idToChar[i] << "    ";
    }
    cout << endl;
    for(int i=0;i<Vcnt;++i){
        cout << idToChar[i] << "   ";
        for(int j=0;j<Vcnt;++j){
            cout << table[i][j] << "   ";
        }
        cout << endl;
    }
}

void initTHead(std::ofstream & fout){
    fout << "#include <string>\n";
    fout << "#include <iostream>\n";
    fout << "#include <assert.h>\n";
    fout << "std::string str;\n";
    fout << "bool isright = true;\n";
    fout << "int p=0;\n";
    fout << "char SYM = '\\0';\n";

    fout << "void advance(){\n";
    fout << "   assert(p<str.length());\n";
    fout << "   SYM = str[p++];\n";
    fout << "}\n";

    fout << "void error(){\n";
    fout << "   isright = false;\n";
    fout << "   std::cout << \"ERROR\" << std::endl;\n";
    fout << "}\n";

    for(auto A : VN){
        fout << "void Procedure_" << A << "();\n";
    }
}

void generateProcedure(std::ofstream & fout, char A){
    fout << "void Procedure_" << A << "(){\n";
    fout << "   if(!isright) return;\n";
    fout << "   switch(SYM){\n";
    for(auto fc : FIRST[charToId[A]]){    // 对于左部为终结符A，依据FIRST(A)，根据当前SYM选择产生式
        int gid = table[charToId[A]][charToId[fc]];
        assert(gid!=-1);
        fout << "       case '" << fc << "':\n";
        for(auto c : Gs[gid].right){    // 按该产生式依次处理
            if(VN.count(c)){    // 为非终结符
                fout << "           Procedure_" << c << "();\n";
            }else{    // 为终结符
                fout << "           if(SYM=='"<< c << "') advance();\n";
                fout << "           else error();\n";
            }
        }
        fout << "           break;\n";
    }
    if(EPSILON[charToId[A]]){    // 假如A可以为空
        fout << "       default:\n";
        fout << "           if(SYM!='"<< DS << "'";
        // 依据FOLLOW(A)进行判断
        for(auto fc : FOLLOW[charToId[A]]){
            fout << "&&SYM!='" << fc << "'";
        }
        fout << ") error();\n";
        fout << "           break;\n";
    }else{
        fout << "       default:\n";
        fout << "           error();\n";
        fout << "           break;\n";
    }
    fout << "   }\n";
    fout << "}\n";
}

void initTProcedures(std::ofstream & fout){
    for(auto A : VN){
        generateProcedure(fout,A);
    }
}

void initTMain(std::ofstream & fout){
    fout << "int main(int argc, const char * argv[]) {\n";
    fout << "   while(std::cin >> str){\n";
    fout << "       p = 0;\n";
    fout << "       advance();\n";
    fout << "       Procedure_" << S << "();\n";
    fout << "       std::cout << isright << std::endl;\n";
    fout << "   }\n";
    fout << "   return 0;\n";
    fout << "}\n";
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "begin to analyze...\n";
    
    readGrammers();
    initEPSILON(S);
    initFIRST(S);
    initFOLLOW(S);
    derivateSELECT();
    printTable();

    // 生成递归下降分析的cpp文件
    std::ofstream fout(Tfile,std::ios::out|std::ios::trunc);

    initTHead(fout);
    initTProcedures(fout);
    initTMain(fout);

    return 0;
}