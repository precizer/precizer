# Copyright 2024-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v3

EAPI=8

DESCRIPTION="Lightweight, high-performance file integrity verification and comparison tool"
HOMEPAGE="https://github.com/precizer/precizer"
SRC_URI="https://github.com/precizer/precizer/archive/refs/tags/${PV}.tar.gz -> v${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~arm64"

DEPEND="
	dev-libs/libpcre2
	dev-db/sqlite:3
"
RDEPEND="${DEPEND}"

S="${WORKDIR}/${P}"

src_compile() {
	emake dynamic-production \
		PROD_CFLAGS='$(CFLAGS)' \
		PROD_LDFLAGS='$(LDFLAGS)' \
		STRIP= \
		UPX=:
}

src_install() {
	dobin precizer
	dodoc README.md
}
