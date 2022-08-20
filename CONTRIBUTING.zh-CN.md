# 向Dragonfly DB提交贡献

非常感谢您能对Dragonfly DB感兴趣。

可以自由查看我们的[讨论区](https://github.com/dragonflydb/dragonfly/discussions)和[issue区](https://github.com/dragonflydb/dragonfly/issues)。

# 最佳实践

## Fork并且Clone DragonflyDB的仓库

```Bash
# Fork https://github.com/dragonflydb/dragonfly

# Clone from your fork
git@github.com:<YOURUSERNAME>/dragonfly.git

cd dragonfly

# IMPORTANT! Enable our pre-commit message hooks
# This will ensure your commits match our formatting requirements
pre-commit install --hook-type commit-msg
```

激活commit-msg 钩子客户端。这一步必须得做，如果你想要开发并提交你的贡献。它能够帮助你提交格式化后的commit msg。

一旦你完成了这些，我们非常期望你能够贡代码并随着Dragonfly DB项目一起成长。

## 从源代码构建

```Bash
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

## 单元测试

```Bash
# Build specific test
cd build-opt && ninja [test_name]
# e.g cd build-opt && ninja generic_family_test 

# Run
./[test_name]
# e.g ./generic_family_test
```

## 签出提交

所有社区提交的内容都必须包含签字。

```Bash
git commit -s -m '...'
```

## 压缩提交

请将所有更改的提交压缩为单个提交（可以使用`"git rebase -i"`命令完成）。请尽量提供格式化过后的提交信息。

## 使用常规提交

本仓库使用[Conventional Commmits](https://www.conventionalcommits.org/en/v1.0.0/)

常规提交规范是提交信息的轻量级约定。它提供了一组简单的规则来创建明确的提交历史；这使得在上面编写自动化工具变得更加容易。通过描述提交代码中所做的功能、修复和重大更改，此约定与 SemVer 相吻合。

提交信息应该具备如下结构：

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

本仓库使用自动化工具标准化代码格式、文本和提交。

[Pre-commit hooks](https://github.com/dragonflydb/dragonfly/blob/main/CONTRIBUTING.md#pre-commit-hooks)将会自动应用代码格式化规则进行验证。

## `pre-commit` hooks

Dragonfly DB 团队已同意系统化地使用一些预提交钩子工具来规范代码格式。您需要安装并启用预提交钩子，才能提交您的代码。

## 贡献许可条款

本项目欢迎贡献、建议和反馈。Dragonfly 使用 BSL 许可证。如果您未拥有代码的版权，您可以在BSL许可证下提交代码。所有反馈、建议或贡献都不是保密的。

## 非常感谢您的贡献