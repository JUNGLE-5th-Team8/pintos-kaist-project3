/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
// #include "userprog/process.h"
#include "threads/malloc.h"
#include "lib/string.h"
#include "threads/mmu.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_content(struct page *page, void *aux);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	lazy_load_info* load_info = calloc(1,sizeof(lazy_load_info));
	if(load_info==NULL){
		return false;
	}
	memcpy(load_info,page->uninit.aux,sizeof(lazy_load_info));

	struct file_page *file_page = &page->file;

	file_page->load_info = load_info; 
	file_page->type = VM_FILE;
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	lazy_load_info* load_info = page->file.load_info;
	file_close(load_info->file);
	free(load_info);
}

/* Do the mmap */
/*
* fd에 해당하는 파일을 offset부터 length 만큼 가상 메모리의 addr에 mapping한다.
* length가 PGSIZE 만큼 정렬되지 못해서 마지막 페이지의 남는 부분은 0으로 채운다.
* written back할 때 0으로 채운 부분은 버린다.
* 성공시 file이 mapping된 virtual adddress 반환, 실패시 NULL return
 */
static bool
lazy_load_content(struct page *page, void *aux)
{
	printf("lazy_loadZZZZZZZZZZZZZZZ!\n");
   if (page == NULL)
   {
      return false;
   }

   lazy_load_info *info = (lazy_load_info *)aux;

   file_seek(info->file, info->offset);
   // 파일에서 페이지를 읽어 메모리에 로드한다.
   if (file_read(info->file, page->frame->kva, info->read_bytes) != (int)info->read_bytes)
   {
    //   printf("lazyload 읽기 실패\n"); // debug
      return false; // 파일 읽기 실패
   }
   memset(page->frame->kva + info->read_bytes, 0, info->zero_bytes);
   free(aux);
   page -> is_loaded = true;
   // 페이지 테이블에 페이지를 추가한다.
   return true;
}

//test_main_page, 4096, 0, handle, 0
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
			//console input이나 output descriptor는 null return하도록 구현.

			//lazy load segment와 lazy_load_info를 이용해서 파일을 읽을 수 있을듯.
			//page struct에 start_address추가
			//type은 vm_file
			size_t remain_bytes = length;
			size_t page_read_bytes = 0;
			size_t page_zero_bytes = 0;
			
			void* cur_addr = addr;
			for(off_t offset_all = offset; offset_all<(offset+length); offset_all+=page_read_bytes){
				page_read_bytes = remain_bytes < PGSIZE ? remain_bytes : PGSIZE;
				page_zero_bytes = PGSIZE-page_read_bytes;
				// printf("%d!!!!!!!!!!!!!!!!!!!!\n",page_read_bytes); debug

				lazy_load_info* lazy_load_aux = calloc(1,sizeof(lazy_load_info));
				lazy_load_aux->file = file_reopen(file); //재오픈 해야할수도
				lazy_load_aux->offset = offset_all;
				lazy_load_aux->read_bytes = page_read_bytes;
				lazy_load_aux->zero_bytes = page_zero_bytes;
				lazy_load_aux->start_addr = addr;

				if(!vm_alloc_page_with_initializer(VM_FILE,cur_addr,writable,lazy_load_content,lazy_load_aux)){
					// printf("ㅋㅋㅋ\n");
					return false;
				}
				// printf("ㅋㅋㅋ\n");
				remain_bytes -= page_read_bytes;
				cur_addr+=PGSIZE;
			}
			return addr;
}

static bool
munmap_check_addr(void *addr, struct supplemental_page_table* spt){
	if(addr != pg_round_down(addr)){
		exit(-1);
	}
	struct hash_elem* first_page_elem = NULL;
	struct page* first_page = NULL;
	if(!(first_page_elem = spt_find_page(spt,addr))){
		first_page = page_entry_from_hash_elem(first_page_elem);
		if(!first_page->start_addr == addr){
			return false;
		}
	}
}

/* Do the munmap */
// close나 file을 지우는 것은 mapping을 지우지 않는다. 일단 만들어지면 mapping이 유지해야 하기 때문에
//\을 사용해 각 파일에 대한 매핑에 대해 독립적인 참조를 유지한다.
void
do_munmap (void *addr) {
	//우선 addr이 제대로 됬는지 확인한다. addr에 대한 page가 존재하는지 확인
	//이것에 대한 page가 file_page이고 첫번째 addr과 일치하는지 확인.

	//frame이 NULL인지 확인하고, pml4_is_dirty를 통해 write back(file_write_at)을 수행할지 확인
	//file back은 페이지를 할당할 때 마다 reopen해서 필드에 추가
	//각자 open된 파일에 대해 close
	//spt_remove_page를 통해서 제거, destory로 할당되어 있던 애들 제거하고 free(page)
	struct supplemental_page_table* spt = &thread_current()->spt;

	if(!munmap_check_addr(addr,spt)){
		exit(-1);
	}

	struct page* page_to_munmap = page_entry_from_hash_elem(spt_find_page(spt,addr));
	void* start_addr = page_to_munmap->start_addr;
	while(page_to_munmap != NULL && page_to_munmap->start_addr == start_addr){
		if(page_to_munmap->frame==NULL){
			if(pml4_is_dirty(thread_current()->pml4, page_to_munmap->va)){
				lazy_load_info* load_info = page_to_munmap->file.load_info;
				// file_write_at(load_info->file,page_to_munmap->frame->kva
				// 		,load_info->read_bytes,load_info->offset);
			}
		}
		spt_remove_page(spt,page_to_munmap);
		addr+=PGSIZE;
		page_to_munmap = spt_find_page(spt,addr);
	}
}
