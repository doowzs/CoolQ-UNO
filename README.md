## CQ UNO 插件（疑似弃坑）
### 基本功能

- 标准UNO游戏
- 伤害叠加
- 抢牌
- 挂机监测

### 如何使用

- 下载并编译为.dll动态链接库
- 用CQ打包成.crx

### 配置文件

文件位于./app/com.doowzs.cquno/config.ini

键值表：

*admin*

- admin：管理员ID

*rule*（0为关闭，1为启用）

- freedraw：自由摸牌
- crazydraw：无法出牌时是否摸到有牌为止
- regressive：是否可以叠加
- seveno：是否启用0-7换牌规则
- jumpin：是否启用抢牌规则