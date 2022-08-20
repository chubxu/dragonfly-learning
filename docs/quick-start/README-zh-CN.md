<p align="center">
  <a href="https://dragonflydb.io">
    <img src="https://raw.githubusercontent.com/dragonflydb/dragonfly/main/.github/images/logo-full.svg"
      width="284" border="0" alt="Dragonfly">
  </a>
</p>

# Quick Start

使用docker启动和运行DragonflyDB是最简单的方法。

如果你的机器上还没安装docker，可以参考这个[链接](https://docs.docker.com/get-docker/)进行安装。

# Step 1

```Bash
docker run --network=host --ulimit memlock=-1 docker.dragonflydb.io/dragonflydb/dragonfly
```

Dragonfly DB允许使用http协议或者Redis协议（RESP）进行连接

你可以使用Redis客户端（redis-cli）连接或者直接通过浏览器访问[http://localhost:6379](http://localhost:6379)

**NOTE：**使用一些配置时，使用`docker run --privileged ...`命令运行能够解决一些初始化的错误。

# Step 2

通过Redis客户端进行连接：

```Bash
redis-cli
127.0.0.1:6379> set hello world
OK
127.0.0.1:6379> keys *
1) "hello"
127.0.0.1:6379> get hello
"world"
127.0.0.1:6379> 
```

# Step 3

请开始体验、使用DragonflyDB的强大功能吧！

# Known issues

#### `Error initializing io_uring`

出现上述错误可能是你的操作系统的内核版本相对较低，请确保内核版本支持`io_uring`能力。

# 更多构建选项

- [Docker Compose Deployment](https://github.com/dragonflydb/dragonfly/blob/main/contrib/docker)
- [Kubernetes Deployment with Helm Chart](https://github.com/dragonflydb/dragonfly/blob/main/contrib/charts/dragonfly)
- [Build From Source](https://github.com/dragonflydb/dragonfly/blob/main/docs/build-from-source.md)