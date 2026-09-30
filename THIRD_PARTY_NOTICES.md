# Third-Party Notices

The [root MIT License](LICENSE) applies to AriaTutorial's own code and prose.
Third-party components retain their respective licenses.

[dependencies.json](dependencies.json) records the source-fetch resolutions
below. An explicit `ARIA_ROOT` or installed Aria SDK can select different
versions; the actual SDK and linked libraries govern a binary's dependencies.
See the [dependency guide](docs/dependencies.md) and
[English update guide](docs/dependency-updates.en.md).

| Component | Current source resolution | License | Use |
|---|---|---|---|
| [Aria](https://github.com/dqsjqian/Aria) | 3.1.1 | MIT | Framework used by the demos. |
| [Mira](https://github.com/dqsjqian/Mira) | 1.0.0 | MIT | Optional HTTP chapter through Aria. |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.12.0 | MIT | Optional HTTP adapter's compiled JSON implementation. |
| [OpenSSL](https://github.com/openssl/openssl) | 4.0.3 | Apache-2.0 | Optional HTTP/TLS configuration; bundled source builds use static libraries. |
| [Qt Core, Gui and Widgets](https://doc.qt.io/qt-6/licensing.html) | Selected installed Qt 6 SDK | LGPL-3.0-only, with GPL/commercial alternatives as specified upstream | Optional Qt chapter and platform plugins. |
| [doctest](https://github.com/doctest/doctest) | 2.5.3 | MIT; embedded portions under Boost-1.0 | Available for Aria tests; embedded Aria tests are disabled in the tutorial build. |

The default tutorial build leaves the Qt and HTTP chapters disabled. A shared
resolution entry alone does not mean its library is present in every demo.

Aria's [LICENSE](https://github.com/dqsjqian/Aria/blob/a56ad396433f5278fba81d920ddd86d1fe5aa923/LICENSE)
and third-party notices apply to the selected framework. Mira's
[LICENSE](https://github.com/dqsjqian/Mira/blob/9386d89d2a259303a0a2c3b1539a5cb0e3fc0be5/LICENSE)
credits Copyright (c) 2026 dqsjqian. nlohmann/json's
[LICENSE.MIT](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/LICENSE.MIT)
credits Copyright (c) 2013–2025 Niels Lohmann, with additional MIT attributions
for Evan Nemerson (2016–2021, Hedley), Florian Loitsch (2009, `to_chars`),
Björn Hoehrmann (2008–2009), and The Abseil Authors (2018).
Mira 1.0.0 supplies its own third-party inventory; the HTTP integration keeps
WebSocket, HTTP/2 and HTTP/3 disabled, so their zlib/ng-series dependencies are
not included. TLS continues to use OpenSSL.
OpenSSL's
[LICENSE.txt](https://github.com/openssl/openssl/blob/af1775b60dfa141a4ad762585052cabeb9f37e9e/LICENSE.txt)
contains Apache-2.0; the selected source archive has no separate `NOTICE` file.
Its bundled Text::Template 1.56 is a build-only tool under GPL-1.0-or-later or
Artistic-1.0 and is not linked into the demos.

## Distribution boundary

The source repository does not include downloaded Aria/dependency caches or a
Qt SDK. Binary demo distribution is a separate step: include full applicable
licenses and attributions for code compiled into each executable and for any
accompanying libraries. A link to upstream alone does not replace those copies.

The Windows runtime deployment in `cmake/TutorialRuntime.cmake` and
`cmake/DeployTutorialRuntime.cmake` can copy Aria/Qt DLLs, Qt platform plugins,
and MinGW runtime DLLs beside the demos. All demo builds stage this project's
licenses and the selected Aria source's known notices (or the selected SDK's
`ARIA_LICENSE_DIR` contents) under `licenses/`. HTTP source builds additionally
copy the configured Mira/JSON/OpenSSL source licenses and `NOTICE*`/`THIRD_PARTY_NOTICES*` files,
including all JSON header SPDX attributions in UTF-8; unknown parent-target
sources produce a warning. An older SDK without license
materials produces a warning. This is not a complete third-party license
bundle or Qt corresponding-source/relinking package. The
current CI runs these binaries but does not upload a demo binary artifact.

Qt under LGPLv3 requires specified notices and copies of LGPLv3 and GPLv3.
Section 4(d)(1) permits an appropriate replaceable shared-library mechanism;
static Qt/plugin linking can use section 4(d)(0) with corresponding library
source and application materials suitable for relinking. Distributing Qt
binaries also requires a permitted route for their corresponding source and
applicable build/modification materials; installation information may be
required in the circumstances stated by the license. The tutorial's MIT source
alone is not a claim that all of those materials have been provided. Consult
[LGPLv3](https://doc.qt.io/qt-6/lgpl.html), [GPLv3 section 6](https://doc.qt.io/qt-6/gpl.html)
and the actual SDK's
[third-party notices](https://doc.qt.io/qt-6/third-party-libraries.html).

Copied compiler/runtime files keep their own terms, including the
[GCC Runtime Library Exception](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html)
where applicable and
[Microsoft runtime redistribution conditions](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170).
Review the actual release file set and selected SDK rather than treating all
DLLs as MIT or assuming a local debug build is ready for public distribution.
