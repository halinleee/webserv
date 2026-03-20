#include "Utils.hpp"
#include "type.hpp"

/**
 * @fn EnvMap envpParsing(char **envp)
 * @brief envp 전체를 std::map<string, string>으로 파싱해서 추가하는 함수
 * @param envp 환경변수를 담고 있는 char **버퍼
 * @return 파싱된 환경변수들을 담은 EnvMap 객체
 */
EnvMap envpParsing(char **envp)
{
    int i = 0;
    EnvMap env;
    
    while (envp[i])
    {
        std::string envbuffer(envp[i]);
        size_t pos = envbuffer.find('=');
        if (pos != std::string::npos)
        {
            std::string key = envbuffer.substr(0, pos);
            std::string value = envbuffer.substr(pos + 1);
            env[key] = value;
        }
        i++;
    }
    return (env);
}

/**
 * @brief 환경변수 맵에 새로운 환경변수를 파싱하여 추가하는 함수
 * @param env 환경변수가 저장될 EnvMap의 참조자
 * @param envAdd 추가할 환경변수 문자열 ("KEY=VALUE" 형식)
 */
void envAdd(EnvMap &env, char *envAdd)
{
    std::string envbuffer(envAdd);
    size_t pos = envbuffer.find('=');
    if (pos != std::string::npos)
    {
        std::string key = envbuffer.substr(0, pos);
        std::string value = envbuffer.substr(pos + 1);
        env[key] = value;
    }
}

/**
 * @brief EnvMap 형식의 환경변수 컨테이너를 환경변수 char** 배열로 변환하는 함수 (주로 CGI 프로그램의 envp로 사용)
 * @param env 변환할 환경변수 맵 (const 참조자)
 * @return 동적 할당된 환경변수 char** 배열 (사용 후 freeSplit으로 메모리 해제 필요, 마지막 요소는 NULL)
 */
char **mapToEnvp(const EnvMap &env)
{
    int i = 0;
    char **envp = new char *[env.size() + 1];
    std::string envpBuffer;
    EnvMap::const_iterator itr;
    for (itr = env.begin(); itr != env.end(); itr++)
    {
        envpBuffer = itr->first + '=' + itr->second;
        envp[i] = new char[envpBuffer.length() + 1];
        std::strcpy(envp[i], envpBuffer.c_str());
        i++;
    }
    envp[i] = NULL;
    return (envp);
}

/**
 * @brief 동적 할당된 2차원 문자열 배열(char**)의 메모리를 해제하는 함수
 * @param tmp 메모리를 해제할 char** 배열
 */
void freeSplit(char **tmp)
{
    int i = 0;

    if (!tmp)
        return ;
    while (tmp[i])
    {
        delete [] tmp[i];
        i++;
    }
    delete []tmp;
}
