#pragma once

#include <google/protobuf/service.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace rdk {
namespace rpc {

template<class TResponse>
class Future : public google::protobuf::Closure {
public:
  Future(std::unique_ptr<TResponse> && p) : m_response(std::move(p)) { }
  Future(const Future &rhs) = delete;
  Future& operator=(const Future &rhs) = delete;

  void Run() override
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_is_done = true;
    m_cond.notify_all();
  }

  operator TResponse *() { return m_response.get(); }

  template<class Rep, class Period>
  std::future_status WaitFor(const std::chrono::duration<Rep,Period>& timeout_duration)
  {    
    std::unique_lock<std::mutex> lock(m_mutex);
    auto result = m_cond.wait_for(lock, timeout_duration, [this] { return m_is_done; });
    return result ? std::future_status::ready : std::future_status::timeout;    
  }

  inline const TResponse& GetResponse() { return *m_response.get(); }

private:
  bool m_is_done { false };
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::unique_ptr<TResponse> m_response;
};

template<class TResponse>
Future<TResponse> make_future(TResponse* response)
{
  return Future<TResponse>(std::unique_ptr<TResponse>(response));
}


class Controller : public google::protobuf::RpcController {
public:  
  void Reset() override;
  bool Failed() const override;
  std::string ErrorText() const override;
  void StartCancel() override;
  void SetFailed(const std::string& reason) override;  
  bool IsCanceled() const override;
  void NotifyOnCancel(google::protobuf::Closure *callback) override;
  void SetTimeout(std::chrono::milliseconds timeout);

private:
  std::string m_error_text;
  bool m_is_canceled { false };
  std::chrono::milliseconds m_timeout = { std::chrono::milliseconds(1000) };
};

} }