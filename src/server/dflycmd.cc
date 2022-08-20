// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.
//
#include "server/dflycmd.h"

#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>

#include "base/flags.h"
#include "base/logging.h"
#include "facade/dragonfly_connection.h"

#include "server/engine_shard_set.h"
#include "server/error.h"
#include "server/journal/journal.h"
#include "server/server_state.h"
#include "server/transaction.h"

using namespace std;

ABSL_DECLARE_FLAG(string, dir);

namespace dfly {

using namespace facade;
using namespace std;
using util::ProactorBase;

DflyCmd::DflyCmd(util::ListenerInterface* listener, journal::Journal* journal) : listener_(listener), journal_(journal) {
}

void DflyCmd::Run(CmdArgList args, ConnectionContext* cntx) {
  DCHECK_GE(args.size(), 2u);

  ToUpper(&args[1]);
  RedisReplyBuilder* rb = static_cast<RedisReplyBuilder*>(cntx->reply_builder());

  std::string_view sub_cmd = ArgS(args, 1);
  if (sub_cmd == "JOURNAL") {
    if (args.size() < 3) {
      return rb->SendError(WrongNumArgsError("DFLY JOURNAL"));
    }
    HandleJournal(args, cntx);
    return;
  }

  if (sub_cmd == "THREAD") {
    util::ProactorPool* pool = shard_set->pool();

    if (args.size() == 2) {  // DFLY THREAD : returns connection thread index and number of threads.
      rb->StartArray(2);
      rb->SendLong(ProactorBase::GetIndex());
      rb->SendLong(long(pool->size()));
      return;
    }

    // DFLY THREAD to_thread : migrates current connection to a different thread.
    std::string_view arg = ArgS(args, 2);
    unsigned num_thread;
    if (!absl::SimpleAtoi(arg, &num_thread)) {
      return rb->SendError(kSyntaxErr);
    }

    if (num_thread < pool->size()) {
      if (int(num_thread) != ProactorBase::GetIndex()) {
        listener_->Migrate(cntx->owner(), pool->at(num_thread));
      }

      return rb->SendOk();
    }

    rb->SendError(kInvalidIntErr);
    return;
  }

  rb->SendError(kSyntaxErr);
}


void DflyCmd::HandleJournal(CmdArgList args, ConnectionContext* cntx) {
  DCHECK_GE(args.size(), 3u);
  ToUpper(&args[2]);

  std::string_view sub_cmd = ArgS(args, 2);
  Transaction* trans = cntx->transaction;
  DCHECK(trans);
  RedisReplyBuilder* rb = static_cast<RedisReplyBuilder*>(cntx->reply_builder());

  if (sub_cmd == "START") {
    unique_lock lk(mu_);
    journal::Journal* journal = ServerState::tlocal()->journal();
    if (!journal) {
      string dir = absl::GetFlag(FLAGS_dir);
      journal_->StartLogging(dir);
      trans->Schedule();
      auto barrier_cb = [](Transaction* t, EngineShard* shard) { return OpStatus::OK; };
      trans->Execute(barrier_cb, true);

      // tx id starting from which we may reliably fetch journal records.
      journal_txid_ = trans->txid();
    }

    return rb->SendLong(journal_txid_);
  }

  if (sub_cmd == "STOP") {
    unique_lock lk(mu_);
    if (journal_->EnterLameDuck()) {
      auto barrier_cb = [](Transaction* t, EngineShard* shard) { return OpStatus::OK; };
      trans->ScheduleSingleHop(std::move(barrier_cb));

      auto ec = journal_->Close();
      LOG_IF(ERROR, ec) << "Error closing journal " << ec;
      journal_txid_ = trans->txid();
    }

    return rb->SendLong(journal_txid_);
  }

  string reply = UnknownSubCmd(sub_cmd, "DFLY");
  return rb->SendError(reply, kSyntaxErrType);
}

}  // namespace dfly
