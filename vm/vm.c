/* vm.c: Generic interface for virtual memory objects. */

#include "lib/string.h"
#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/mmu.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "vm/uninit.h"
#include "vm/anon.h"
#include "threads/vaddr.h"

#include "vm/file.h"
static bool child_copy_pm(struct page* page, void* aux);
static bool spt_hash_less_func (const struct hash_elem *a,const struct hash_elem *b,void *aux);
static uint64_t spt_hash_func(struct hash_elem* supplemental_hash_elem);


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
//로드 때 선언하는 
//vm_initializer은 lazy_load_segment, load_segment의 인수는 aux
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page* new_page = NULL;
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		// printf("vm_alloc_page_with_initializer : vm type : %#x va : %#x down: %#x\n",type,upage,pg_round_down(upage));//debug
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		//uninit_new를 호출한 이후에 page fault시 필드를 바꿔야 한다는 뜻인가
		//아니면 여서 바로 바꿔야 한다는 뜻인가?
		//근데 uninit_new로 이미 선언 해주었는데 여기서는 당장 수정할게 없을 것 같은데
		new_page = calloc(1,sizeof(struct page));
		if(new_page==NULL) PANIC("can't calloc!");
		bool (*page_initializer) (struct page *, enum vm_type, void *kva);
		void* start_addr = NULL;

		if(VM_TYPE(type) == VM_ANON){
			page_initializer = anon_initializer;
		}
		else if(VM_TYPE(type)==VM_FILE){
			lazy_load_info* info = (lazy_load_info*)aux;
			page_initializer == file_backed_initializer;
			start_addr = info->start_addr;
		}
		else{
			PANIC("type isn't exits");
		}

		upage = pg_round_down(upage);
		uninit_new(new_page,upage,init,type,aux,page_initializer);
		new_page -> writable = writable;
		new_page -> is_loaded = false;
		new_page-> start_addr = start_addr;
		// printf("vm_alloc_page_with_initializer : page va : %d\n",new_page->va);//debug
		/* TODO: Insert the page into the spt. */
		if(!spt_insert_page(spt,new_page)){
			return false;
		}
		return true;
	}
	return false;
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page page_same_va;
	struct page *page = NULL;
	page_same_va.va = pg_round_down(va);

	struct hash_elem *hash_elem = hash_find(&spt-> hash_table, &page_same_va.spt_hash_elem);
	if(hash_elem!=NULL){
		page = page_entry_from_hash_elem(hash_elem);
	}
	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// printf("spt_insert_page va :%d \n",page->va);//debug
	if(spt_find_page(spt,page->va)==NULL) {
		hash_insert(spt,&page->spt_hash_elem);
		return true;
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	struct hash_elem* hash_elem_removed = hash_delete(spt, &page->spt_hash_elem);
	if(hash_elem_removed==NULL){
		printf("spt_remove_page spt에 page가 없음.");
		exit(-1);
	}
	free(page->frame);
	vm_dealloc_page (page);
	return ;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = malloc(sizeof(struct frame));
	frame->page = NULL;
	frame-> kva = palloc_get_page(PAL_USER|PAL_ZERO);
	// printf("%p\n",vtop(frame-> kva));
	if(frame->kva == NULL){
		PANIC("todo");
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	void* address_bottom = pg_round_down(addr);
	while(!spt_find_page(&thread_current()->spt,address_bottom)){
		vm_alloc_page(VM_ANON||8,address_bottom,true);
		vm_claim_page(address_bottom);
		address_bottom+=PGSIZE;	
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}
/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	// printf("vm_try_handle_fault addr %p\n",addr);
	if(!(page = spt_find_page(spt,addr))){
		// printf("vm_try_handle_fault : page가 존재하지 않음. %#x\n",addr);
		// printf("원인은 무엇인지 kenerl or user %d\n",user);
		//page가 없는 경우 stack 영역인지 확인
		//fault addr rsp보다 8만큼 아래거나, rsp를 이미 내린 상태에서 fault가 발생한 경우 처리
		if(user && addr< USER_STACK && addr>=(USER_STACK-(PGSIZE<<8)) && (f->rsp-8==addr || f->rsp<=addr)){
			vm_stack_growth(addr);
			return true;
		}
		return false;
		
	}
	if(write&&!page->writable) return false;	
	// else{
	// 	printf("vm_try_handle_fault : 존재하는 주소에 접근하는건데 addr %#x down addr%#x\n",addr,pg_round_down(addr));//debug
	// 	printf("vm_try_handle_fault : 찾은 페이지의 주소%#x\n",page->va);
	// 	printf("anon? %#x\n",VM_TYPE(page->anon.type)==VM_ANON);//debug
	// }

	//spt_find_page를 통하여 fault address에 해당하는 페이지 구조체 얻기
	if(!vm_do_claim_page (page)){
		printf("vm_do_claim_page (page) 걸림\n");
		return false;
	}

	return true;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
//가상 메모리에 해당하는 페이지를 사용하기 위해 세팅하는 것.
//가상 메모리에는 실질적인 물리적 메모리에는 할당되어있지 않은 상태
//해당하는 가상메모리에 프레임을 할당하고 이것을 페이지와 매핑한다.
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;

	/* TODO: Fill this function */
	// 여기서는 가상 메모리에 해당하는 페이지를
	// supplimentary page table에서 가져오고 넘겨야겠지
	page = spt_find_page(&thread_current()->spt,va);
	if(page == NULL){
		printf("vm_claim_page : page를 찾을 수 없음\n");
		return false;
	}

	return vm_do_claim_page (page);
}

// /* Create a PAGE of stack at the USER_STACK. Return true on success. */
/* Claim the PAGE and set up the mmu. */
//vm get page로 얻은 프레임을 page와 매핑
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	/* Set links */
	if(frame==NULL){
		printf("vm_do_claim_page : frame이 존재하지 않음.\n");
		return false;
	}
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	//페이지 테이블에 페이지 테이블 엔트리를 삽입하여 va를 프레임의 pa와 mapping
	// printf("%p!!!!!!!!!!!!!!!!!!!!!!!!!\n",USER_STACK- (int64_t)(page->va));
	if(!pml4_set_page(thread_current()->pml4, page->va , frame->kva, page->writable)){
		printf("vm_do_claim_page : pml4_set_page fail 물리주소와 가상 주소 매핑 실패\n");
		return false;
	}
	// printf("vm_do_claim_page : 왔다\n"); debug
	
	//물리 메모리를 공간을 만들고 page table entry와 연결했으면 
	//page 정보를 통해 할당된 공간을 채워줘야 겠지.
	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash_table,spt_hash_func,spt_hash_less_func,NULL);
}
static
bool spt_hash_less_func (const struct hash_elem *a,const struct hash_elem *b,void *aux){
	void* a_va = page_entry_from_hash_elem(a)->va;
	void* b_va = page_entry_from_hash_elem(b)->va;
	return a_va < b_va;
}

//hash elem으로부터 page return, hash_elem이 null일 경우 null return
struct page* page_entry_from_hash_elem(struct hash_elem* supplemental_hash_elem){
	if(supplemental_hash_elem==NULL){
		return NULL;
	}
	return hash_entry(supplemental_hash_elem,struct page,spt_hash_elem);
}

static uint64_t spt_hash_func(struct hash_elem* supplemental_hash_elem){
	// printf("!!!!!!!!!!!!\n");
	if(supplemental_hash_elem==NULL){
		PANIC("NO!!!!!!!!!!!!!!!!!!!!!!");
	}
	int spt_va = page_entry_from_hash_elem(supplemental_hash_elem)->va;
	return hash_int(spt_va);
	// void* spt_va = page_entry_from_hash_elem(supplemental_hash_elem)->va;
	// return hash_bytes(spt_va,sizeof(void*));
}

//부모의 메모리로부터 자식의 메모리에 채워넣는 함수
static bool
child_copy_pm(struct page* page, void* aux){
	page->is_loaded =true;
	memcpy(page->frame->kva,aux,PGSIZE);
	return true;
}
/* Copy supplemental page table from src to dst */
/*부모 page의 load여부 확인 : loaded상태라면 vn_initilzer에 
lazyload_segment가 아니라 부모 page의 kva를 통해 그곳에 있는 내용을
자식의 할당된 부분에 채워넣게 구현, 아니라면 부모의 page struct를 통해 채워넣기*/
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	struct hash_iterator i;

	hash_first (&i, &src->hash_table);
	while (hash_next (&i))
	{
		struct page *page_to_copy = page_entry_from_hash_elem(hash_cur(&i));
		//로드된 경우
		if(page_to_copy->is_loaded){//init메모리가 아니라면....
			vm_alloc_page_with_initializer(page_to_copy->anon.type,page_to_copy->va,
			page_to_copy->writable,child_copy_pm,page_to_copy->frame->kva);//NULL에 메모리 로드하는 함수 만들기
			//부모에서 미리 메모리에 할당되있던 곳들은 claim
			vm_claim_page(page_to_copy->va);
		}
		//로드가 안 된 경우
		else{
			//자식에게 전달할 aux
			void* aux = malloc(sizeof(lazy_load_info));
			memcpy(aux,page_to_copy->uninit.aux,sizeof(lazy_load_info));
			//부모의 페이지로부터 자식 페이지 복사
			vm_alloc_page_with_initializer(page_to_copy->uninit.type,page_to_copy->va,
			page_to_copy->writable,page_to_copy->uninit.init,aux);
		}
	}
	return true;
}

void free_page(struct hash_elem* elem, void *aux){
	struct page *page_to_del = page_entry_from_hash_elem(elem);
	destroy(page_to_del);
	free(page_to_del->frame);
	free(page_to_del);
}
/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	//spt에 있는 page 구조체들만 제거하는 것만 구현
	hash_clear(spt,free_page);
}
