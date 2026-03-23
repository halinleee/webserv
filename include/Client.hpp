#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "main.hpp" // 차후에 main.hpp를 제거하고 필요한 client에 관련된 헤더파일은 hpp에서 직접 include 하도록 수정
#include "Socket.hpp"

/**
 * @brief getPipeFd 함수에서 이 플레그를 전달해 CGI에서 http의 요청에서 body을 전달하는 inPipe의 쓰기 끝을 반환하는 flag
 */
#define InFlag 0
/**
 * @brief getPipeFd 함수에서 이 플레그를 전달해 CGI에서 생성된 http요청을 받는 OutPipe의 읽기 끝을 반환하는 flag
 */
#define OutFlag 1

/**
 * @brief 서버에 연결된 단일 클라이언트의 정보와 상태를 관리하는 클래스
 *  클라이언트 소켓, 수신 버퍼, 환경변수, CGI 파이프 및 PID 등을 저장합니다.
 */
class Client
{
    private:
        /**
         * @param clientSocket 클라이언트의 소켓 FD, addr(port, ip주소등 관련 정보), time(마지막 요청 시간)을 가지고 있는 Socket클래스
         */
        Socket *clientSocket;
        /**
         * @param recVec recv로 수신받은 내용을 담고있는 <char> 백터
         */
        CharVec recVec;
        /**
         * @param env 서버에서 복사한 환경변수에 CGI의 Meta-variable 및 http 요청의 쿼리 스트링을 추가하여 가지고 있는 <string, string>맵
         *  http의 요청 파싱 후 Meta-variable 및 http 요청의 쿼리 스트링 추가하는 로직 필요
         */
        EnvMap env;
        /**
         * @param inPipe CGI 프로그램으로 내용을 보내는 pipe의 FD를 가지고 있는 size 2의 int배열
         */
        int inPipe[2];
        /**
         * @param outPipe CGI 프로그램으로 내용을 받아오는 pipe의 FD를 가지고 있는 size 2의 int배열
         */
        int outPipe[2];
        /**
         * @param statusCode http respose를 보낼때 필요한 statusCode
         *  코드를 확인하여 각 동작으로 넘어가는 분기과정 필요
         * ex = 200, 404, 405
         */
        int statusCode;
        /**
         * @param pid CGI프로그램의 프로세스 id
         *  CGI프로그램 동작 후 자원 회수를 하기 위해서 사용
         */
        pid_t pid;
    
    public:
        /**
         * @brief Client의 기본 생성자
         *  에러 방지로 선언 사용 X
         */
        Client();

        /**
         * @brief Client 객체의 private변수를 초기화하는 생성자
         * @param socket 클라이언트와 연결된 Socket 객체의 포인터 관련 정보가 담겨있는 addr과 마지막 사용시간인 time도 가지고 있음
         * @param env 클라이언트에 할당할 환경변수 맵(서버에서 복사한 기본 환경변수)
         */
        Client(Socket *socket, EnvMap env);

        /**
         * @brief CGI 실행 시 할당된 자식 프로세스의 PID를 설정하는 함수
         * @param pid 자식 프로세스 PID
         *  CGI 프로그램 구동 후 자원회수를 하기위해 사용
         */
        void setPid(pid_t pid);

        /**
         * @brief CGI 통신을 위한 파이프 FD를 설정하는 함수
         * @param inPipe 입력용 파이프 배열
         * @param outPipe 출력용 파이프 배열
         */
        void setPipeFd(int inPipe[2], int outPipe[2]);

        /**
         * @brief 클라이언트의 statuscode를 설정하는 함수
         */
        void setStatusCode(int statusCode);

        /**
         * @brief recv로 수신된 데이터를 버퍼(recvec<char> 백터)에 추가하는 함수
         * @param length 수신된 데이터의 길이
         * @param received 수신된 데이터 배열
         */
        void charVecAppend(int length, char *received);

        /**
         * @brief 클라이언트의 수신 버퍼를 반환하는 함수
         * @return CharVec 참조
         */
        CharVec &getCharVec(void);

        /**
         * @brief 클라이언트의 소켓 객체를 반환하는 함수
         * @return Socket 객체 참조
         */
        Socket &getSocket();

        /**
         * @brief CGI 자식 프로세스의 PID를 반환하는 함수
         * @return 프로세스의 PID
         */
        pid_t getPid(void);

        /**
         * @brief 클라이언트의 statuscode를 반환
         * @return 클라이언트의 statusCode
         */
        int getStatusCode();

        /**
         * @brief Pipe의 FD를 반환하는 함수
         * @param index Inflag = inPipe의 쓰기끝, OutFlag = outPipe의 읽기끝
         * @return 파이프의 FD
         */
        int getPipeFd(int index);
        /**
         * @brief Socket클래스의 자원을 회수하는 함수
         *  client의 pipe, 소켓의 fd, 소켓 delete까지 수행
         */
        void delSocket();
        /**
         * @brief pipe를 close하는 함수
         * @param pipe pipeFD를 담고 있는 size 2인 int 배열
         */
        void pipeClose(int *pipe);
};


#endif