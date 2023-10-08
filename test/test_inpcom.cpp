#include <iostream>
#include <string>
#include <mutex>
#include "inpcom/inpcom_simo.hpp"
using namespace fast_com::ipc;

std::mutex g_mtx;

struct test_message
{
    int index;
    std::string str;
};

void egomotion_result_inpcom_callback(std::shared_ptr<test_message> message)
{
    std::lock_guard<std::mutex> mtx(g_mtx);
    std::cout << "[INFO][advertise_1]receive message: [index] ->" << message->index << "  [str] -> " << message->str << std::endl;  
}

void egomotion_result_inpcom_callback_1(std::shared_ptr<test_message> message)
{
    std::lock_guard<std::mutex> mtx(g_mtx);
    std::cout << "[INFO][advertise_2]receive message: [index] ->" << message->index << "  [str] -> " << message->str << std::endl;  
}



int main()
{
    InpcomSIMOPtr inpcom_node_ = InpcomSIMOPtr(InpcomSIMO::get_instance("test_message"));
    std::shared_ptr<InpcomSIMO::Publisher<test_message>> publisher = inpcom_node_->advertise<test_message>("/test/message");
    std::shared_ptr<InpcomSIMO::Subscriber<test_message>> subscriber = inpcom_node_->subscribe<test_message>("/test/message", egomotion_result_inpcom_callback);
    std::shared_ptr<InpcomSIMO::Subscriber<test_message>> subscriber1 = inpcom_node_->subscribe<test_message>("/test/message", egomotion_result_inpcom_callback_1);
    while (true)
    {
        
        std::shared_ptr<test_message> message = std::make_shared<test_message>();
        static int index = 1;
        static std::string str = "hello!";

        message->index = index;
        message->str = str + "[" + std::to_string(index) + "]";
        {
            std::lock_guard<std::mutex> mtx(g_mtx);
            publisher->publish(message);
        }
        std::cout << "[INFO][publish]publish message: [index] ->" << message->index << "  [str] -> " << message->str << std::endl;
        sleep(1);
        index++;
    }
    return 0;
}