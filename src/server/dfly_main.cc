
// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.
//

#ifdef NDEBUG
#include <mimalloc-new-delete.h>
#endif

// Abseil flag库允许以编程方式访问通过命令行传递给二进制文件的flag值。
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
// absl/strings 库提供了用于操作和比较字符串的类和实用函数
#include <absl/strings/match.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>

// 这是 io_uring 库
#include <liburing.h>

// 它是 malloc 的直接替代品，可以在其他程序中使用而无需更改代码
#include <mimalloc.h>

// signal.h是C标准函数库中的信号处理部分， 定义了程序执行时如何处理不同的信号。信号用作进程间通信， 报告异常行为（如除零）、用户的一些按键组合（如同时按下Ctrl与C键，产生信号SIGINT）。
#include <signal.h>

// 以下文件来自于helio和src目录下
#include "base/init.h"
#include "base/proc_util.h"  // for GetKernelVersion
#include "facade/dragonfly_listener.h"
#include "io/file.h"
#include "io/file_util.h"
#include "io/proc_reader.h"
#include "server/main_service.h"
#include "server/version.h"
#include "strings/human_readable.h"
#include "util/accept_server.h"
#include "util/uring/uring_pool.h"
#include "util/varz.h"

using namespace std;

// 声明启动文件接收的参数，声明后可全局使用
ABSL_DECLARE_FLAG(uint32_t, port);
ABSL_DECLARE_FLAG(uint32_t, dbnum);
ABSL_DECLARE_FLAG(uint32_t, memcache_port);
ABSL_DECLARE_FLAG(uint64_t, maxmemory);

// 定义全局flag
ABSL_FLAG(bool, use_large_pages, false, "If true - uses large memory pages for allocations");
ABSL_FLAG(string, bind, "",
          "Bind address. If empty - binds on all interfaces. "
          "It's not advised due to security implications.");
ABSL_FLAG(string, pidfile, "", "If not empty - server writes its pid into the file");
ABSL_FLAG(string, unixsocket, "",
          "If not empty - specifies path for the Unis socket that will "
          "be used for listening for incoming connections.");

using namespace util;
using namespace facade;
using namespace io;
using absl::GetFlag;
using absl::StrCat;
using strings::HumanReadableNumBytes;

namespace dfly {
  namespace {

    enum class TermColor { 
      kDefault, 
      kRed, 
      kGreen, 
      kYellow 
    };

    // 获取颜色编码
    // 在bash中: \033[0;31m、\033[0;32m、\033[0;33m分别代表红色、绿色、黄色编码
    // 可以通过如下代码 yellow='\033[0;33m'; echo -e '${yellow}hello'; 输出具有颜色的文本
    static const char* GetAnsiColorCode(TermColor color) {
      switch (color) {
        case TermColor::kRed:
          return "1";
        case TermColor::kGreen:
          return "2";
        case TermColor::kYellow:
          return "3";
        default:
          return nullptr;
      }
    }
    string ColorStart(TermColor color) {
      return StrCat("\033[0;3", GetAnsiColorCode(color), "m");
    }
    // 重置终端颜色为默认颜色
    const char kColorEnd[] = "\033[m";
    // 获取有颜色的文本
    string ColoredStr(TermColor color, string_view str) {
      return StrCat(ColorStart(color), str, kColorEnd);
    }

    bool HelpshortFlags(std::string_view f) {
      return absl::StartsWith(f, "\033[0;32");
    }

    bool HelpFlags(std::string_view f) {
      return absl::StartsWith(f, "\033[0;3");
    }

    string NormalizePaths(std::string_view path) {
      if (absl::ConsumePrefix(&path, "../src/"))
        return ColoredStr(TermColor::kGreen, path);

      if (absl::ConsumePrefix(&path, "../"))
        return ColoredStr(TermColor::kYellow, path);

      if (absl::ConsumePrefix(&path, "_deps/"))
        return string(path);

      return string(path);
    }

    bool RunEngine(ProactorPool* pool, AcceptServer* acceptor) {
      auto maxmemory = GetFlag(FLAGS_maxmemory);

      if (maxmemory > 0 && maxmemory < pool->size() * 256_MB) {
        LOG(ERROR) << "Max memory is less than 256MB per thread. Exiting...";
        return false;
      }

      Service service(pool);

      Listener* main_listener = new Listener{Protocol::REDIS, &service};

      Service::InitOpts opts;
      opts.disable_time_update = false;
      service.Init(acceptor, main_listener, opts);
      const auto& bind = GetFlag(FLAGS_bind);
      const char* bind_addr = bind.empty() ? nullptr : bind.c_str();
      auto port = GetFlag(FLAGS_port);
      auto mc_port = GetFlag(FLAGS_memcache_port);
      string unix_sock = GetFlag(FLAGS_unixsocket);
      bool unlink_uds = false;

      if (!unix_sock.empty()) {
        unlink(unix_sock.c_str());

        Listener* uds_listener = new Listener{Protocol::REDIS, &service};
        error_code ec = acceptor->AddUDSListener(unix_sock.c_str(), uds_listener);
        if (ec) {
          LOG(WARNING) << "Could not open unix socket " << unix_sock << ", error " << ec;
          delete uds_listener;
        } else {
          LOG(INFO) << "Listening on unix socket " << unix_sock;
          unlink_uds = true;
        }
      }

      error_code ec = acceptor->AddListener(bind_addr, port, main_listener);

      LOG_IF(FATAL, ec) << "Could not open port " << port << ", error: " << ec.message();

      if (mc_port > 0) {
        acceptor->AddListener(mc_port, new Listener{Protocol::MEMCACHE, &service});
      }

      acceptor->Run();
      acceptor->Wait();

      service.Shutdown();

      if (unlink_uds) {
        unlink(unix_sock.c_str());
      }

      return true;
    }

    bool CreatePidFile(const string& path) {
      Result<WriteFile*> res = OpenWrite(path);
      if (!res) {
        LOG(ERROR) << "Failed to open pidfile with error: " << res.error().message() << ". Exiting...";
        return false;
      }

      unique_ptr<WriteFile> wf(res.value());
      // 写入pid
      auto ec = wf->Write(to_string(getpid()));
      if (ec) {
        LOG(ERROR) << "Failed to write pid into pidfile with error: " << ec.message() << ". Exiting...";
        return false;
      }

      ec = wf->Close();
      if (ec) {
        LOG(WARNING) << "Failed to close pidfile file descriptor with error: " << ec.message() << ".";
      }

      return true;
    }

  }  // namespace

}  // namespace dfly

extern "C" void _mi_options_init();

using namespace dfly;

void sigill_hdlr(int signo) {
  LOG(ERROR) << "An attempt to execute an instruction failed."
             << "The root cause might be an old hardware. Exiting...";
  exit(1);
}

int main(int argc, char* argv[]) {
  // 设置帮助报告例程使用的“使用”消息。
  absl::SetProgramUsageMessage(
      R"(a modern in-memory store.

Usage: dragonfly [FLAGS]
)");

  absl::FlagsUsageConfig config;
  config.contains_help_flags = dfly::HelpFlags;
  config.contains_helpshort_flags = dfly::HelpshortFlags;
  config.normalize_filename = dfly::NormalizePaths;
  config.version_string = [] {
    string version = StrCat(dfly::kGitTag, "-", dfly::kGitSha);
    return StrCat("dragonfly ", ColoredStr(TermColor::kGreen, version),
                  "\nbuild time: ", ColoredStr(TermColor::kYellow, dfly::kBuildTime), "\n");
  };

  absl::SetFlagsUsageConfig(config);

  MainInitGuard guard(&argc, &argv);

  // 输出版本号日志
  LOG(INFO) << "Starting dragonfly " << GetVersion() << "-" << kGitSha;

  struct sigaction act;
  act.sa_handler = sigill_hdlr;
  sigemptyset(&act.sa_mask);
  sigaction(SIGILL, &act, nullptr);

  CHECK_GT(GetFlag(FLAGS_port), 0u);
  mi_stats_reset();

  base::sys::KernelVersion kver;
  base::sys::GetKernelVersion(&kver);

  // 校验内核版本号，不了解的可以搜下Linux内核版本号的定义规则
  if (kver.kernel < 5 || (kver.kernel == 5 && kver.major < 10)) {
    LOG(ERROR) << "Kernel 5.10 or later is supported. Exiting...";
    return 1;
  }

  // io_uring_queue_init_params 初始化 io_uring 功能
  int iouring_res = io_uring_queue_init_params(0, nullptr, nullptr);
  if (-iouring_res == ENOSYS) {
    LOG(ERROR) << "iouring system call interface is not supported. Exiting...";
    return 1;
  }
  // 校验DB数量，最大1024
  if (GetFlag(FLAGS_dbnum) > dfly::kMaxDbId) {
    LOG(ERROR) << "dbnum is too big. Exiting...";
    return 1;
  }

  // 校验内核版本号
  CHECK_LT(kver.major, 99u);
  dfly::kernel_version = kver.kernel * 100 + kver.major;

  // 判断pidfile参数是否存在
  string pidfile_path = GetFlag(FLAGS_pidfile);
  if (!pidfile_path.empty()) {
    // 参数存在则创建文件，并写入pid，如果创建失败则直接返回
    if (!CreatePidFile(pidfile_path)) {
      return 1;
    }
  }
  // 判断最大内存参数
  if (GetFlag(FLAGS_maxmemory) == 0) {
    LOG(INFO) << "maxmemory has not been specified. Deciding myself....";
    // 读取内存信息
    Result<MemInfoData> res = ReadMemInfo();
    size_t available = res->mem_avail;
    // 使用80%的可用内存作为dragonfly的最大内存
    size_t maxmemory = size_t(0.8 * available);
    LOG(INFO) << "Found " << HumanReadableNumBytes(available)
              << " available memory. Setting maxmemory to " << HumanReadableNumBytes(maxmemory);
    absl::SetFlag(&FLAGS_maxmemory, maxmemory);
  } else {
    LOG(INFO) << "Max memory limit is: " << HumanReadableNumBytes(GetFlag(FLAGS_maxmemory));
  }
  // 设置最大内存
  dfly::max_memory_limit = GetFlag(FLAGS_maxmemory);
  // 如果设置了使用大页
  if (GetFlag(FLAGS_use_large_pages)) {
    mi_option_enable(mi_option_large_os_pages);
  }
  mi_option_enable(mi_option_show_errors);
  mi_option_set(mi_option_max_warnings, 0);

  // 创建一个 ring_depth=1024 的io_uring pool，pool size跟随系统
  uring::UringPool pp{1024};

  // 运行 io_uring pool
  // UringPool -> ProactorPool，这里的run方法运行的是 proactor_pool.cc 中的Run()方法
  pp.Run();

  AcceptServer acceptor(&pp);

  // 运行后端服务器
  int res = dfly::RunEngine(&pp, &acceptor) ? 0 : -1;

  pp.Stop();

  if (!pidfile_path.empty()) {
    unlink(pidfile_path.c_str());
  }

  return res;
}
