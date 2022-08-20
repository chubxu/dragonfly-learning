<p align="center">
  <a href="https://dragonflydb.io">
    <img  src="/.github/images/logo-full.svg"
      width="284" border="0" alt="Dragonfly">
  </a>
</p>

[![ci-tests](https://github.com/dragonflydb/dragonfly/actions/workflows/ci.yml/badge.svg)](https://github.com/dragonflydb/dragonfly/actions/workflows/ci.yml) [![Twitter URL](https://img.shields.io/twitter/follow/dragonflydbio?style=social)](https://twitter.com/dragonflydbio)


[Quick Start](https://github.com/dragonflydb/dragonfly/tree/main/docs/quick-start) | [Discord Chat](https://discord.gg/HsPjXGVH85) | [GitHub Discussions](https://github.com/dragonflydb/dragonfly/discussions) | [GitHub Issues](https://github.com/dragonflydb/dragonfly/issues) | [Contributing](https://github.com/dragonflydb/dragonfly/blob/main/CONTRIBUTING.md)

# 有可能，这是宇宙中最快的内存数据库

Dragonfly是一款现代内存数据库，与 Redis 和 Memcached API 完全兼容。Dragonfly 在多线程、无共享架构之上实现了**新的算法与数据结构**。因此，Dragonfly相较于Redis 拥有25倍性能的提升，并且单个Dragonfly实例能够支持数百万 QPS。

Dragonfly的核心特性使其成为高效、高性能并易于使用的Redis替代品。

# 基准测试

<img src="http://assets.dragonflydb.io/repo-assets/aws-throughput.svg" width="80%" border="0"/>

在亚马逊云上的c6gn.16xlarge实例上（64CPU、128G内存、100Gbps网络带宽、38Gbps吞吐量），Dragonfly超过3.8M的QPS，相较于Redis，吞吐量增加了25倍。

Dragonfly 峰值吞吐量的P99在不同机器上的值如下表所示：

| op  |r6g | c6gn | c7g |
|-----|-----|------|----|
| set |0.8ms  | 1ms | 1ms   |
| get | 0.9ms | 0.9ms |0.8ms |
|setex| 0.9ms | 1.1ms | 1.3ms

*ps：所有基准测试均使用 memtier_benchmark执行，并且标明了每个服务类型、实例类型和相应的线程数。memtier 在单独的 c6gn.16xlarge 机器上运行。对于setex的基准测试，我们将超时时间设置为500，因此，所有设置的key都能够在基准测试中存活。*

在pipeline模式下（--pipeline=30），Dragonfly的SET操作达到10M的QPS，GET操作达到15M的QPS。

## Memcached / Dragonfly

在亚马逊的c6gn.16xlarge 实例上，我们还对比了memcached和Dragonfly。正如下面表格所示，Dragonfly在读写负载方面，吞吐量高于memcached，并且延迟相当。对于写负载，因为memcached的写路径竞争问题，Dragonfly还有更好的延迟性能。

### SET benchmark

|Server|QPS(thousands qps)|latency 99%|99.9%|
|-|-|-|-|
|Dragonfly|🟩 3844|🟩 0.9ms|🟩 2.4ms|
|Memcached|806|1.6ms|3.2ms|


### GET benchmark

|Server|QPS(thousands qps)|latency 99%|99.9%|
|-|-|-|-|
|Dragonfly|🟩 3717|1ms|2.4ms|
|Memcached|2100|🟩 0.34ms|🟩 0.6ms|

Memcached 在get基准测试中表现出较低的延迟，但吞吐量也较低。

## 内存效率

在接下来的测试中，我们分别使用Dragonfly 和 Redis填充了大约5GB的数据到内存中（使用`debug populate 5000000 key 1024`命令）。然后我们使用memtier发送更新请求，同时利bgsave命令同步数据。下图清晰的展示了两台服务器在内存效率方面的表现。

<img src="http://assets.dragonflydb.io/repo-assets/bgsave-memusage.svg" width="70%" border="0"/>

在空闲状态下，Dragonfly 的内存效率比 Redis 高 30%。并且在数据同步期间，Dragonfly也几乎看不见任何可见的内存增长。于此同时，Redis在峰值情况下，内存大约增至3倍。在同步速度方面，Dragonfly同样更为快速，仅仅用了几十秒。对于内存效率对比的更多信息可以参考[dashtable doc](https://github.com/chubxu/dragonfly/blob/main/docs/dashtable.md)。

# 服务器运行

Dragonfly运行在Linux上，它使用了Linux相对较新的io-uring API进行I/O操作。因此，他需要Linux5.10+。Debian/Bullseye, Ubuntu 20.04.4+来满足这些需求。

## docker运行

```Bash
docker run --network=host --ulimit memlock=-1 docker.dragonflydb.io/dragonflydb/dragonfly

redis-cli PING  # redis-cli can be installed with "apt install -y redis-tools"
```

你需要添加`*`--ulimit memlock=-1`*`命令，因为Linux 发行版将容器的默认 memlock 限制配置为 64m，而 Dragonfly 需要更多。

## Releases版本

我们维护了x86和arm64架构的二进制版本。您需要安装 libunwind8 库才能运行二进制文件。

## 从源代码构建

在Ubuntu 20.04+的机器上，你需要安装如下依赖进行构建。

```Bash
git clone --recursive https://github.com/dragonflydb/dragonfly && cd dragonfly

# to install dependencies
sudo apt install ninja-build libunwind-dev libboost-fiber-dev libssl-dev \
     autoconf-archive libtool cmake g++

# Configure the build
./helio/blaze.sh -release

# Build
cd build-opt && ninja dragonfly

# Run
./dragonfly --alsologtostderr
```

# 配置

在适用的情况下，Dragonfly 支持常见的 redis 参数。例如，你可以运行如下脚本：`dragonfly --requirepass=foo --bind localhost`。

Dragonfly 目前支持以下 Redis 特定参数：

- `port`
- `bind`
- `requirepass`
- `maxmemory`
- `dir`：默认情况下，dragonfly的docker镜像使用`/data`目录作为快照目录。你可以使用 `-v` docker选项来映射你的主机目录。
- `dbfilename`

此外，Dragonfly还有自己的特定的参数选项：

- `memcache_port`：在此端口上启动memcached兼容API，默认未启动
- `keys_output_limit`：使用`keys`命令时，返回的最大数量。默认是8192。`keys`是一个不安全的命令，我们对该命令进行截断，防止返回太多的keys导致内存溢出。
- `dbnum`：select 支持的最大数据库数。
- `cache_mode`：查看[Cache](https://github.com/chubxu/dragonfly#novel-cache-design) 章节
- `hz`：keys的过期检查频率， 默认为1000。低频率使用更少的CPU，但是会增长空闲key的保留时间。

对于更多的选项，运行 `dragonfly --help`命令查看。

# Roadmap and status

当前的Dragonfly支持130个Redis命令和所有的memcache命令包括`cas`。我们几乎兼容了Redis 2.8的所有API。我们的第一个里程碑将是稳定当前的基本功能并与 Redis 2.8 和 Memcached API 实现 API 对等。如果你发现了你需要的API，但是目前还未实现，请提交issue告知我们。

下一个里程碑将是实现redis -> dragonfly以及dragonfly<->dragonfly的副本集以实现dragonfly的高可用能力。

对于dragonfly的本地复制，我们计划设计一种分布式日志格式，在复制时支持更高数量级的复制速度。

在完成复制和失败转移的功能后，我们将继续实现其它的Redis API，包括Redis3至5版本。

查阅[API readiness doc](https://github.com/chubxu/dragonfly/blob/main/docs/api_status.md)文档来查看dragonfly的当前状态

## 高可用里程碑

实现主从复制

## "Maturity"里程碑

实现Redis3、4、5版本的API，但不包括集群、模块、内存自省（memory introspection）的命令。同样也不会包括geo命令、keyspace notifications、流处理命令。可能会支持配置。总体来说会有几十个命令。可能实现集群 API 装饰器以允许集群配置的客户端连接到单个实例。

接下来的里程碑将在此过程中确定。

# 设计决定

## 新颖的缓存设计

Dragonfly 有一个统一的自适应缓存算法，非常简单且内存高效。您可以通过传递 `--cache_mode=true` 参数来启用缓存模式。一旦该模式被启动，Dragonfly 仅会在将要达到最大内存限制时，删除未来最不可能偶然发现的key。

## 相对精确的过期期限

过期时间限制在4年内。此外，对于超过 134217727 毫秒（大约 37 小时）的截止日期，毫秒精度的到期期限（PEXPIRE/PSETEX 等）将四舍五入到最接近的秒数。这种舍入的误差小于 0.001%，我希望这对于大范围是可以接受的。如果您不能接收，请提交issue告诉我您的解释。

有关此实现与 Redis 实现之间的更详细的差异请[看这里](https://github.com/chubxu/dragonfly/blob/main/docs/differences.md)。

## 原生的Http console并兼容Prometheus的指标

默认情况下，Dragonfly 允许通过其主 TCP 端口 (6379) 进行 http 访问。没错，你可以通过 Redis 协议（REpl）和 HTTP 协议连接到 Dragonfly——服务器在连接发起时会自动识别协议。现在就用你的浏览器试试吧。目前它没有太多信息，但未来我们计划添加有用的调试和管理能力。如果你访问`:6379/metrics`url，你将会看到一些兼容Prometheus的监控指标。

Prometheus 导出的指标与 Grafana 兼容，具体参考[这里](https://github.com/chubxu/dragonfly/blob/main/examples/grafana/dashboard.json)。

注意！Http控制台推荐在安全网络中访问。如果你向外部暴露了Dragonfly的TCP端口，推荐你关闭控制台的访问功能。你可以通过`--http_admin_console=false`或者`--nohttp_admin_console`参数关闭。

# 背景

Dragonfly最初是一项实验项目，旨在了解如果在 2022 年设计内存数据存储，将会成为什么样子。根据我们作为内存存储用户和为云公司工作的工程师的经验教训，我们知道我们需要为 Dragonfly 保留两个关键属性：a)：为所有操作提供原子性保证；b)：在高吞吐量下保证极低的亚毫秒延迟。

我们的第一个挑战是如何充分使用云服务器上的CPU、内存、IO资源。为了解决这个问题，我们使用了非共享架构，它允许我们在线程之间对内存中的键空间进行分区，以便每个线程可以管理自己的字典数据。我们称这些为slices - shards。可以在这个[开源项目](https://github.com/romange/helio)中查看为非共享架构提供的线程和IO管理库。

为了保证多key操作下的原子性，我们参考了学术研究的最新进展。我们选择了论文“[VLL: a lock manager redesign for main memory database systems](https://www.cs.umd.edu/~abadi/papers/vldbj-vll.pdf)”来开发 Dragonfly 的事务框架。非共享架构和VLL的组合允许我们在不使用互斥锁和自旋锁的情况下解决多key场景下的原子性问题。这是我们 PoC 的一个重要里程碑，其性能在其他商业和开源解决方案中脱颖而出。

我们第二个挑战是为存储开发一种新的数据结构。为了实现这一目标，我们基于论文[“Dash: Scalable Hashing on Persistent Memory”](https://arxiv.org/pdf/2003.07302.pdf)构建了核心的哈希表结构。论文本身以持久内存为中心，与主内存存储没有直接关系。尽管如此，它却非常适用于解决我们的问题。它提出了一种哈希表设计，允许我们实现 Redis 字典中存在的两个特殊能力：a)：在数据存储增长期间的增量哈希能力；b)：能够在更新操作下无状态扫描字典。除了这两个属性之外，Dash 在 CPU 和内存方面的效率要高得多。通过利用 Dash 的设计，我们能够通过以下功能进一步创新：

- 高效记录TTL的到期时间
- 一种新颖的缓存回收算法，它比其他缓存策略（如 LRU 和 LFU）实现更高的命中率，且内存开销为零。
- 一种新颖的无分叉快照算法

在我们实现了 Dragonfly 的基础功能，并对其性能感到满意后，我们继续实现 Redis 和 Memcached 的相关功能。到目前为止，我们已经实现了大约 130 个 Redis 命令（相当于 v2.8）和 13 个 Memcached 命令。

最后：

*我们的使命是利用最新硬件构建一个能够支撑云上作业的设计精良、超快、经济高效的内存数据存储。我们打算解决当前解决方案的痛点，同时保留其产品 API 和特点。*