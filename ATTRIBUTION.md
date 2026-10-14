# Attribution and changes / 来源及修改

## Source and contributors / 来源与贡献者

- **Original application:** [darrenprx/lolPing](https://github.com/darrenprx/lolPing), by **Danrui Meng**. Its source code is distributed under the [MIT license](LICENSE), whose original copyright notice is retained here.
- **Linux X11 port:** [upstream pull request #60](https://github.com/darrenprx/lolPing/pull/60), authored by **[995916639a-coder](https://github.com/995916639a-coder)**. This repository was prepared from that PR's head commit [`b50a72662e802a793646468c8ad239b8c0604c8b`](https://github.com/darrenprx/lolPing/commit/b50a72662e802a793646468c8ad239b8c0604c8b), based on upstream commit [`f24a51e5a11c2b31dab72d89a571cc4025ad3c98`](https://github.com/darrenprx/lolPing/commit/f24a51e5a11c2b31dab72d89a571cc4025ad3c98). The PR was not merged when this edition was prepared.
- **This community edition:** maintained in [TaDiao/lolPing-linux](https://github.com/TaDiao/lolPing-linux). It is independent of the original project and is not an official upstream release. The original author and the Linux PR author retain credit for their respective work.

中文：本项目基于 Danrui Meng 的原版 lolPing，以及 995916639a-coder 提交的实验性 Linux X11 适配 PR #60。原项目版权声明和 MIT 许可证保留在 [`LICENSE`](LICENSE) 中。本仓库是独立的社区发布，不代表原作者或 PR 作者发布。

## What came from the Linux PR / Linux PR 已完成的工作

PR #60 added the X11 input helper, Linux packaging and build scripts, Linux desktop integration and settings, documentation, CI and Linux tests. The Linux adaptation should be attributed to its author; this repository does not claim that work as a new implementation.

中文：X11 输入辅助程序、Linux 打包与构建、桌面集成和设置、相关文档、CI 与测试，均来自 PR #60。本仓库不将这些工作称为自行原创。

## Changes made for this edition / 本版本另做的修改

Compared with the exact PR head above, the initial release changes were limited to:

1. Add `-lXext` to the Linux helper link command in [`native/hook-helper/build.sh`](native/hook-helper/build.sh), fixing the missing X11 extension library during packaging.
2. Add repository-specific attribution, release instructions and Linux package notes to the English and Chinese READMEs and [`docs/linux-release.md`](docs/linux-release.md).
3. Remove the upstream Windows/macOS automatic release workflow from this Linux edition, so its tag does not publish unrelated installers.
4. Build and publish the experimental x86-64 `.deb` and AppImage from this source. These are packaging outputs, not a new Linux port. The Linux package configuration now includes the original `LICENSE` and this attribution file inside both installers.

The repository's later documentation updates clarify the Linux release process and this attribution. The source repository was uploaded as a snapshot, so its Git history does not include the original upstream commits; the exact source commits above provide that provenance.

中文：相对于上述 PR 版本，本仓库补上 Linux 链接参数 `-lXext`，增加本仓库的中英文署名与发布说明，移除原版 Windows/macOS 自动发布工作流，并构建、发布实验性的 x86-64 `.deb` 和 AppImage。新版 Linux 安装包内也加入原始 `LICENSE` 和本署名文件。后续文档更新进一步澄清发布流程和来源。由于本仓库以源码快照上传，Git 提交历史未包含上游提交；上方列出的提交可供核对来源。

## Code and game assets / 代码与游戏素材

The MIT license applies to the application's source code, **not** to League of Legends icons, emotes, animations or sounds. Those assets belong to Riot Games and are subject to [Riot's Legal Jibber Jabber policy](https://www.riotgames.com/en/legal) and other applicable terms. This edition is offered free of charge as a community fan project. Check the current Riot policy before redistributing its source assets or installers; attribution alone does not grant a license to Riot's assets.

**Riot notice:** lolPing-linux was created under Riot Games' "Legal Jibber Jabber" policy using assets owned by Riot Games. Riot Games does not endorse or sponsor this project.

中文：MIT 许可证仅适用于应用源码，不涵盖《英雄联盟》的图标、表情、动画或音效；这些素材归 Riot Games 所有，使用仍须遵守其政策及相关条款。本项目免费作为社区粉丝作品发布。转载源码素材或安装包前，请核对 Riot 的现行政策；注明来源本身不等于获得素材授权。Riot Games 不认可或赞助本项目。
