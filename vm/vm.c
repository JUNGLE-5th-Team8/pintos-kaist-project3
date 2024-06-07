/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/lib/kernel/hash.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"
#include "string.h"
#include "userprog/process.h"

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
	// printf("\nvm_alloc_page_with_initializer 시작 \n"); /* Debug */
	// printf("vm_type : %d\n", type);						/* Debug */
	// printf("upage : %p\n", upage);						/* Debug */
	// printf("writable : %d\n\n", writable);				/* Debug */

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		struct page *new_page = malloc(sizeof(struct page));

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			uninit_new(new_page, upage, init, type, aux, anon_initializer);
			new_page->start_address = NULL;
			break;
		case VM_FILE:
			uninit_new(new_page, upage, init, type, aux, file_backed_initializer);
			new_page->start_address = ((struct auxillary *)aux)->start_address;
			break;
		default:
			break;
		}

		// printf("uninit page 구조체 생성 : %p\n", upage); /* Debug */
		new_page->writable = writable;
		new_page->is_loaded = false;

		/* Insert the page into the spt. */
		if (spt_insert_page(spt, new_page))
		{
			// printf("spt 에 uninit page구조체 삽입 성공\n\n"); /* Debug */
			return true;
		}
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

	// printf("비교할 페이지 구조체 생성 va: %p\n", va); /* Debug */

	struct hash_elem *hash_elem = hash_find(&spt->hash_table, &page_to_compare_va.hash_elem);
	if (hash_elem != NULL)
	{
		// printf("spt에 hash_elem이 있음\n\n"); /* Debug */
		page = hash_entry(hash_elem, struct page, hash_elem);
	}
	else
	{
		// printf("spt에 hash_elem이 없음\n\n"); /* Debug */
	}

	return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *hash_elem = hash_insert(&spt->hash_table, &page->hash_elem);
	// null 포인터를 반환한 경우 해시테이블에 페이지를 삽입한다.
	// 이미 요소가 있는 경우 페이지를 삽입하지 않고, 이미 존재하는 요소 반환
	if (hash_elem == NULL)
	{
		succ = true;
	}
	// printf("spt insert page 에서 spt에 hash_elem 삽입 성공(1) 실패(0) : %d\n", succ); /* Debug */
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
	void *paddr = palloc_get_page(PAL_USER | PAL_ZERO);

	// 메모리가 꽉찼을 경우 swap out을 처리해줘야하지만 일단 panic(todo)로 케이스만 표시하고 넘어감.
	if (paddr == NULL)
	{
		PANIC("todo");
	}

	frame = (struct frame *)malloc(sizeof(struct frame)); // 프레임 할당
	frame->kva = paddr;									  // 프레임 구조체 초기화
	frame->page = NULL;

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
	void *addr_bottom = pg_round_down(addr);
	/* 주소의 bottom이 스택한계를 넘으면 stack overflow */

	while (!spt_find_page(&thread_current()->spt, addr_bottom))
	{
		/* 스택 프레임 할당 */
		if (vm_alloc_page(VM_MARKER_0 | VM_ANON, addr_bottom, true))
		{
			vm_claim_page(addr_bottom);
		}
		addr_bottom += PGSIZE;
	}
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
	// printf("\npagefault 발생 addr : %p\n", addr); /* Debug */
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	/* user가 kernel영역에 접근한 경우 */
	if (user && is_kernel_vaddr(addr))
	{
		// printf("유저가 커널영역에 접근\n"); /* Debug */
		return false;
	}

	// printf("pg_round_up(f->rsp) : %p\n", pg_round_up(f->rsp));	   /* Debug */
	// printf("pg_round_down(f->rsp) : %p\n", pg_round_down(f->rsp)); /* Debug */
	// printf("f->rsp : %p\n", f->rsp);							   /* Debug */

	page = spt_find_page(spt, addr);
	if (page == NULL)
	{

		/* stack 증가(sp감소)로 인해서 발생한 pagefault일 때
		 * 1. push로 인한 스택 증가 : 현재 스택포인터 다음 주소를 검사함 (rsp - 8 == addr)
		 * 2. 함수 호출로 인한 스택 증가 : 스택포인터를 함수크기만큼 증가시키고 push함 (rsp <= addr)
		 */
		if (STACK_BOTTOM <= addr && ((f->rsp - 8) == addr || f->rsp <= addr) && addr <= USER_STACK)
		{
			// printf("stack으로 인한 페이지폴트"); /* Debug */
			vm_stack_growth(addr);
			return true;
		}
		// if (!user && ((thread_current()->saved_rsp - 8) == addr || thread_current()->saved_rsp <= addr) && addr <= USER_STACK)
		// {
		// 	vm_stack_growth(addr);
		// 	return true;
		// }
		else
			// printf("페이지를 찾을 수 없음\n"); /* Debug */
			return false;
	}

	/* read-only에 write시도 */
	if (write && !page->writable)
	{
		// printf("read-only에 write시도\n"); /* Debug */
		return false;
	}

	if (vm_do_claim_page(page))
	{
		// printf("vm_do_claim 성공\n"); /* Debug */
		return true;
	}
	else
	{
		// printf("do_claim 실패\n"); /* Debug */
		return false;
	}
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
	// printf("vm_do_claim_page 시작 \npage_type : %d\n", page->uninit.type); /* Debug */
	struct frame *frame = vm_get_frame();

	/* Set links */
	page->frame = frame;
	frame->page = page;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// MMU 세팅: 가상 주소와 물리 주소를 매핑한 정보를 페이지 테이블에 추가해야한다.
	// unchecked : 매핑하는 함수가 맞는지 확실친 않음

	// unchecked : 기본코드에 적혀있어서 일단 살려놨는데 의도를 모르겠음.
	if (pml4_set_page(thread_current()->pml4, page->va, page->frame->kva, page->writable))
	{
		// printf("pml4_set_page 성공\n"); /* Debug */
		// printf("page->va : %p\npage->frame->kva : %p\n\n", page->va, page->frame->kva);/* Debug */
	}
	return swap_in(page, page->frame->kva);
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

bool copy_child(struct page *child, void *aux)
{
	// struct page *parent = aux;
	memcpy(child->frame->kva, aux, PGSIZE);
	return true;
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	/* 해시 이터레이터 생성 */
	struct hash_iterator hi;
	hash_first(&hi, &src->hash_table);

	/* FIXME: dst에 있는 hash table 정리 */
	// hash_clear(&dst->hash_table, )

	/* 이터레이터 순회하면서 복사 */
	struct hash_elem *hash_elem;
	while ((hash_elem = hash_next(&hi)) != NULL)
	{
		/* 부모 페이지 */
		struct page *old_page = hash_entry(hash_elem, struct page, hash_elem);

		/* 이미 로드된 페이지는 즉시 연결 and 복사 */
		if (old_page->is_loaded)
		{
			vm_alloc_page_with_initializer(old_page->anon.type, old_page->va, old_page->writable,
										   copy_child, old_page->frame->kva);
			vm_claim_page(old_page->va);
		}

		/* 로드 안된 페이지는 lazy_load 되도록 부모에서 복사 */
		else
		{
			/* 부모의 aux 복사 */
			struct auxillary *aux = malloc(sizeof(struct auxillary));
			memcpy(aux, old_page->uninit.aux, sizeof(struct auxillary));

			/* 페이지 생성 및 삽입 */
			vm_alloc_page_with_initializer(old_page->uninit.type, old_page->va, old_page->writable,
										   old_page->uninit.init, aux);
		}
	}
	return true;
}

void hash_action_clear(struct hash_elem *hash_e, void *aux)
{
	struct page *page = hash_entry(hash_e, struct page, hash_elem);
	// list_remove(&hash_e->list_elem);
	destroy(page);
	// palloc_free_page(page->frame->kva); pml4에서 알아서 해준다고 함
	free(page->frame);
	free(page);
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	hash_clear(&spt->hash_table, hash_action_clear);
}
