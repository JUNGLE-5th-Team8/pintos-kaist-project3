/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
// #include "userprog/process.h"
#include "threads/malloc.h"
#include "lib/string.h"

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

	struct file_page *file_page = &page->file;
	lazy_load_info* load_info = calloc(1,sizeof(lazy_load_info));
	if(load_info==NULL){
		return false;
	}
	memcpy(load_info,page->uninit.aux,sizeof(lazy_load_info))==NULL;
	
	file_page->load_info = load_info  ;
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
   if (page == NULL)
   {
      return false;
   }

   lazy_load_info *info = (lazy_load_info *)aux;

   file_seek(info->file, info->offset);

   // 파일에서 페이지를 읽어 메모리에 로드한다.
   if (file_read(info->file, page->frame->kva, info->read_bytes) != (int)info->read_bytes)
   {
      // printf("lazyload 읽기 실패\n"); // debug
      return false; // 파일 읽기 실패
   }
   memset(page->frame->kva + info->read_bytes, 0, info->zero_bytes);
   free(aux);
   // 페이지 테이블에 페이지를 추가한다.
   return true;
}

void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
			//파일의 길이가 0인지 확인 file size
			
			if(length==0){
				return NULL;
			}
			
			//addr가 page로 정렬된 부분이 아니라면
			if(addr != pg_round_down(addr)){
				return NULL;
			}

			//map할 위치에 어느 페이지와 주소가 겹친다면
			for(int page_addr = addr; page_addr<(addr+length); page_addr+=PGSIZE){
				if(spt_find_page(&thread_current()->spt,page_addr)!=NULL){
					return NULL;
				}
			}

			//console input이나 output descriptor는 null return하도록 구현.

			//lazy load segment와 lazy_load_info를 이용해서 파일을 읽을 수 있을듯.
			//page struct에 start_address추가
			//type은 vm_file
			size_t remain_bytes = length;
			for(int offset = addr; offset<(addr+length); offset+=PGSIZE){
				size_t page_read_bytes = remain_bytes < PGSIZE ? remain_bytes : PGSIZE;
				size_t page_zero_bytes = PGSIZE-page_read_bytes;

				lazy_load_info* lazy_load_aux = calloc(1,sizeof(lazy_load_info));
				lazy_load_aux->file = file; //재오픈 해야할수도
				lazy_load_aux->offset = offset;
				lazy_load_aux->read_bytes = page_read_bytes;
				lazy_load_aux->zero_bytes = page_zero_bytes;
				lazy_load_aux->start_addr = addr;

				if(vm_alloc_page_with_initializer(VM_FILE,addr,writable,lazy_load_content,lazy_load_aux)){
					return false;
				}

				remain_bytes-=PGSIZE;
			}

			return addr;
}

/* Do the munmap */
// close나 file을 지우는 것은 mapping을 지우지 않는다. 일단 만들어지면 mapping이 유지해야 하기 때문에
//\을 사용해 각 파일에 대한 매핑에 대해 독립적인 참조를 유지한다.
void
do_munmap (void *addr) {

}
