/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/lib/kernel/hash.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		// uint64_t *kva = palloc_get_page(PAL_USER | PAL_ZERO);

		struct page *new_page = malloc(sizeof(struct page));

		uninit_new(new_page, upage, init, type, aux, anon_initializer);

		/* Insert the page into the spt. */
		if (spt_insert_page(spt, upage))
			return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	/* pg_round_down() 으로 va의 페이지 번호를 얻음 -> va가 속한 페이지의 시작 주소를 알기위해서
	 * hash_find() 함수를 사용해서 hash_elem 구조체 얻음
	 * 만약 존재하지 않는다면 null 리턴
	 * hash_entry()로 해당 hash_elem의 page 구조체 리턴*/
	struct page *page = NULL;

	/* TODO: Fill this function. */
	struct page page_to_compare_va;
	va = pg_round_down(va);
	page_to_compare_va.va = va;

	struct hash_elem *hash_elem = hash_find(&spt->hash_table, &page_to_compare_va.hash_elem);
	if (hash_elem != NULL)
	{
		page = hash_entry(hash_elem, struct page, hash_elem);
	}

	return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *hash_elem = hash_insert(spt, &page->hash_elem);
	// null 포인터를 반환한 경우 해시테이블에 페이지를 삽입한다.
	// 이미 요소가 있는 경우 페이지를 삽입하지 않고, 이미 존재하는 요소 반환
	if (hash_elem == NULL)
	{
		succ = true;
	}

	return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	// unchecked : par_zero를 해줘야하는지 확실하진 않음.
	void *paddr = palloc_get_page(PAL_USER);

	// 메모리가 꽉찼을 경우 swap out을 처리해줘야하지만 일단 panic(todo)로 케이스만 표시하고 넘어감.
	if (paddr == NULL)
	{
		PANIC("todo");
	}

	frame = (struct frame *)malloc(sizeof(struct frame)); // 프레임 할당
	frame->kva = paddr;									  // 프레임 구조체 초기화

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	page = spt_find_page(spt, addr);
	if (page == NULL)
	{
		printf("페이지를 찾을 수 없음\n");
		return false;
	}

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
// va가 할당된 페이지에 프레임을 할당하는 함수
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function */
	// unchecked : 확실하지 않음.
	struct thread *t = thread_current();
	page = spt_find_page(&t->spt, va);

	if (page == NULL)
	{
		return false;
	}

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// MMU 세팅: 가상 주소와 물리 주소를 매핑한 정보를 페이지 테이블에 추가해야한다.
	// unchecked : 매핑하는 함수가 맞는지 확실친 않음

	// unchecked : 기본코드에 적혀있어서 일단 살려놨는데 의도를 모르겠음.
	pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable);
	return swap_in(page, frame->kva);
}

/* --------------------------project3 추가 함수 ------------------------------- */

// 입력된 두 hash_elem의 vaddr 비교하는 함수
static bool compare_hash_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	/* hash_entry()로 각각의 element에 대한 page 구조체를 얻은 후 vaddr 비교
	 * a가 b보다 작으면 true
	 * a가 b보다 크거나 같으면 false*/
	struct page *page_a = hash_entry(a, struct page, hash_elem);
	struct page *page_b = hash_entry(b, struct page, hash_elem);
	return page_a->va < page_b->va;
}

// page의 vaddr을 인자값으로 hash_int() 함수를 사용하여 해시 값 반환
static uint64_t get_hash_func(const struct hash_elem *e, void *aux)
{
	/* hash_entry()로 각각의 element에 대한 page 구조체 검색
	 * hash_int()를 이용해서 page의 멤버 vaddr에 대한 해시값을 구하고 반환*/
	struct page *page_a = hash_entry(e, struct page, hash_elem);
	uint64_t hash_value = hash_int(page_a->va);
	return hash_value;
}

/*------------------------project3 추가 함수 끝------------------------------*/

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	// 해시테이블 초기화 진행
	// 인자로 get_hash_func, compare_hash_func 함수 사용
	hash_init(&spt->hash_table, get_hash_func, compare_hash_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
