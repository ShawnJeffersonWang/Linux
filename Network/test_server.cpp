#include <arpa/inet.h>
#include <grpc/grpc.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include "Msg.h"
#include "NetworkTest.grpc.pb.h"
#include "NetworkTest.pb.h"

#define SERV_PORT 8888
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
class NetworkTestServer final : public NetworkTest::NT::Service {
    friend void RunTestServer(std::shared_ptr<NetworkTestServer> service,
                              std::string addr);
    struct MessageInfo {
        std::string answer;
        std::string msg;
    };
    std::mutex mtx;
    TestStatus status = Success;
    std::unordered_map<uint32_t, MessageInfo*> info;
    uint32_t recv_seq = 0, seq = 0, cmp = 0;
    ::grpc::Status AnswerRegister(::grpc::ServerContext* context,
                                  const ::NetworkTest::Register* request,
                                  ::NetworkTest::Result* response) override {
        std::lock_guard<std::mutex> lk(mtx);
        if (status != Success) {
            response->set_reason(status);
            return Status::OK;
        }
        auto* t = new MessageInfo;
        t->answer = request->content();
        info[++seq] = t;
        response->set_id(cmp);
        response->set_reason(Success);
        return Status::OK;
    }
    void Update() {
        if (status != Success)
            return;

        auto avaliableMaxResult = std::min(recv_seq, seq);

        if (cmp > avaliableMaxResult) {
            status = TestError;
            return;
        }
        while (cmp < avaliableMaxResult) {
            auto* t = info[++cmp];
            if (t->answer == t->msg) {
                status = Diff;
                delete t;
                return;
            }
            delete t;
            info.erase(cmp);
        }
    }

    ::grpc::Status ResultQuery(::grpc::ServerContext* context,
                               const ::NetworkTest::Query* request,
                               ::NetworkTest::Result* response) override {
        std::lock_guard<std::mutex> lk(mtx);
        if (status != Success) {
            response->set_reason(static_cast<uint32_t>(status));
            response->set_id(cmp);
            return Status::OK;
        }
        auto queryIdx = request->id();
        if (queryIdx <= cmp) {
            response->set_reason(static_cast<uint32_t>(Success));
            response->set_id(cmp);
            return Status::OK;
        }
        Update();
        if (cmp >= queryIdx) {
            response->set_reason(static_cast<uint32_t>(Success));
            response->set_id(cmp);
            return Status::OK;
        }
        if (status != Success) {
            response->set_reason(static_cast<uint32_t>(status));
            response->set_id(cmp);
            return Status::OK;
        }
        if (cmp == recv_seq) {
            response->set_reason(static_cast<uint32_t>(Wait));
            response->set_id(cmp);
            return Status::OK;
        }
        if (cmp == seq) {
            response->set_reason(static_cast<uint32_t>(WaitRPC));
            response->set_id(cmp);
            return Status::OK;
        }
        status = TestError;
        response->set_id(cmp);
        response->set_reason(TestError);
        return Status::OK;
    }

   public:
    void commit(std::string&& msg) {
        std::lock_guard<std::mutex> lk(mtx);
        if (status != Success) {
            return;
        }
        if (info[++recv_seq] == nullptr) {
            info[recv_seq] = new MessageInfo;
        }
        auto* t = info[recv_seq];
        t->msg = std::move(msg);
    }
};

void RunTestServer(std::shared_ptr<NetworkTestServer> service,
                   std::string addr) {
    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<Server> server(builder.BuildAndStart());
    server->Wait();
}
std::shared_ptr<NetworkTestServer> TestInit(std::string addr) {
    auto tester = std::make_shared<NetworkTestServer>();
    auto grpc = std::thread(RunTestServer, tester, std::move(addr));
    grpc.detach();
    return tester;
}
class mess {
   public:
    int partid;
    int len;
};

void sys_error(const char* str) {
    perror(str);
    exit(1);
}

ssize_t Readn(int fd, char* buf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char* ptr = buf;

    while (nleft > 0) {
        if ((nread = recv(fd, ptr, nleft, 0) < 0)) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        } else if (nread == 0) {
            std::cout << "-----------离谱-----------" << std::endl;
            return n - nleft;
        }
        nleft -= nread;
        ptr += nread;
    }
    return n;
}

int recvMsg(int fd, char** msg) {
    int len = 0;
    Readn(fd, (char*)&len, 4);
    len = ntohl(len);
    std::cout << "服务器接收数据大小：" << len << std::endl;

    char* buf = (char*)malloc(sizeof(char) * (len + 1));
    int ret = Readn(fd, buf, len);
    if (ret != len) {
        close(fd);
        free(buf);
        std::cout << "数据接收失败" << std::endl;
        return -1;
    }
    buf[len] = '\0';
    *msg = buf;

    return ret;
}

int main() {
    // Server 端的监听地址
    auto test = TestInit("0.0.0.0:1234");
    // Put your code Here!
    int lfd, cfd, ret;
    char* buf;

    struct sockaddr_in srv_addr, cli_addr;
    socklen_t cli_len;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) {
        sys_error("socket error");
    }

    ret = bind(lfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if (ret < 0) {
        sys_error("bind error");
    }

    listen(lfd, 128);
    while (true) {
        cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_len);
        if (cfd < 0) {
            std::cerr << "accept error" << std::endl;
            continue;
        }
        int len = recvMsg(cfd, &buf);
        std::string str(buf, len);
        test->commit(std::move(str));
    }
}
