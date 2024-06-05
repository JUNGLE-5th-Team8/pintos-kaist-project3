#ifndef VM_UNINIT_H
#define VM_UNINIT_H
#include "vm/vm.h"

struct page;
enum vm_type;

typedef bool vm_initializer (struct page *, void *aux);

/* Uninitlialized page. The type for implementing the
 * "Lazy loading". */
struct uninit_page {
	/* Initiate the contets of the page */
	vm_initializer *init;
	enum vm_type type; //type에 따라 다른 ininitializer를 호출할 것이다
	void *aux; //page를 unitit page를 초기화 하기 위한 인수(page fault시 사용)
	/* Initiate the struct page and maps the pa to the va */
	//가상 주소와 물리 메모리와 매핑하는 것 같다.
	bool (*page_initializer) (struct page *, enum vm_type, void *kva);
};

void uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *kva));
#endif
