# Copyright 1999-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="Lightweight, high-performance file integrity verification and comparison tool"
HOMEPAGE="https://precizer.github.io/"
SRC_URI="https://github.com/precizer/precizer/archive/refs/tags/${PV}.tar.gz -> ${P}.tar.gz"

S="${WORKDIR}/${P}"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="amd64 arm64"
IUSE="test"
RESTRICT="!test? ( test )"

RDEPEND="
	dev-db/sqlite:3
	dev-libs/libpcre2
"
DEPEND="
	${RDEPEND}
"
DOCS=( README.md CHANGELOG.md )

src_compile() {
	emake distribution
}

src_test() {
	emake tests-dynamic
}

src_install() {
	dobin .builds/distribution/precizer
	einstalldocs
}
