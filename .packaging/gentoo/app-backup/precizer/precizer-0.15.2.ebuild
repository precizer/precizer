# Copyright 2024-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v3

EAPI=8

DESCRIPTION="Lightweight, high-performance file integrity verification and comparison tool"
HOMEPAGE="https://precizer.github.io/"
SRC_URI="https://github.com/precizer/precizer/archive/refs/tags/${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~arm64"
IUSE="test"
RESTRICT="!test? ( test )"

RDEPEND="
	dev-db/sqlite:3
	dev-libs/libpcre2
"
DEPEND="
	${RDEPEND}
"
BDEPEND="
	test? ( llvm-core/llvm )
"

S="${WORKDIR}/${P}"
DOCS=( README.md CHANGELOG.md )

src_compile() {
	emake dynamic-production-build \
		PROD_OPT_CFLAGS='${CFLAGS}' \
		PROD_OPT_LDFLAGS='${LDFLAGS}' \
		STRIP= \
		UPX=:
}

src_test() {
	emake tests-dynamic
}

src_install() {
	dobin .builds/dynamic-production/precizer
	einstalldocs
}
