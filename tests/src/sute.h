#include "precizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "testitall.h"
#include "mem.h"
#include "xdiff.h"
#include "helpers.h"
#include "mocks.h"
#include "testmocking.h"

Return test0003(void);
Return test0004(void);
Return test0005(void);
Return test0006(void);

Return test0009(void);

Return test0011(void);
Return test0012(void);
Return test0013(void);
Return test0014(void);
Return test0015(void);
Return test0016(void);

Return test0018(void);
Return test0019(void);
Return test0020(void);
Return test0021(void);
Return test0022(void);
Return test0023(void);
Return test0024(void);

Return test0026(void);
Return test0027(void);
Return test0028(void);
Return test0029(void);
Return test0030(void);
Return test0031(void);
Return test0033(void);
Return test0034(void);

Return test0036(void);
Return test0037(void);
Return test0038(void);

Return comprehensive_system_testing(void);
Return comprehensive_unit_testing(void);
Return comprehensive_mock_testing(void);
Return function_unit_testing(void);
Return bundled_libraries(void);

/**
 * @brief Prepare the isolated test environment and fixture workspace
 *
 * @return Return status code
 */
Return prepare(void);
Return finish(void);

/**
 * @brief Attempt cleanup of the temporary test workspace under TMPDIR
 *
 * @return Return status code
 */
Return clean(void);
