#include "IMarketDataProvider.h"
// Vtable anchor: forca a unidade de compilacao a gerar o vtable de
// IMarketDataProvider. Sem isso o linker reclama de undefined reference
// quando subclasses (BrapiProvider) usam o construtor base.
