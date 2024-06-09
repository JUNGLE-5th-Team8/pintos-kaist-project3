/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "string.h"
#include "threads/mmu.h"
// #include_next "lib/stdio.h" /* debug */

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
}

/* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	struct auxillary *tmp = malloc(sizeof(struct auxillary));
	memcpy(tmp, page->uninit.aux, sizeof(struct auxillary));

	struct file_page *file_page = &page->file;

	file_page->type = VM_FILE;
	file_page->aux = tmp;

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;

	/* page가 mmap되었고 munmap되지 않았으면 (spt에 존재) */

	/* 프레임이 할당되었고 수정되었으면 파일에 적음 */
	// if (page->start_address == page)
	// {
	if (page->frame != NULL && pml4_is_dirty(thread_current()->pml4, page->va))
	{
		struct auxillary *auxi = file_page->aux;

		// lock_acquire(&filesys_lock);
		// file_seek(auxi->file, auxi->ofs);
		if (file_tell(auxi->file) == auxi->ofs)
		{
			// printf("포지션 같음\n\n");
		}
		off_t write_cnt = file_write_at(auxi->file, page->va, auxi->prb, auxi->ofs);
		// off_t write_cnt = file_write(auxi->file, page->va, auxi->prb);
		// lock_release(&filesys_lock);
	}
	// file_close(((struct auxillary *)(file_page->aux))->file);
	free(file_page->aux);
	// }
}

static bool
lazy_load_contents(struct page *page, void *aux)
{
	// printf("lazy_load_contents\n");
	/* 페이지 확인 */
	if (page == NULL)
	{
		// printf("page 가 null임\n"); /* Debug */
		return false;
	}

	/* aux로 전달받은 변수 */
	struct auxillary *auxi = aux;

	page->is_loaded = true;

	/* 할당된 페이지에 로드 */
	// lock_acquire(&filesys_lock);
	file_seek(auxi->file, auxi->ofs);
	off_t result = file_read(auxi->file, page->frame->kva, auxi->prb);
	// lock_release(&filesys_lock);

	// printf("page_read_bytes : %d\n", page_read_bytes);
	// printf("result : %d\n", result);
	if (result != auxi->prb)
	{
		printf("file_read 실패 \n"); /* Debug */
		return false;
	}

	/* 페이지의 남은 부분을 0으로 채우기 */
	memset(page->frame->kva + auxi->prb, 0, auxi->pzb);

	/* aux 동적할당 해제 */
	// free(aux);
	return true;
}

/* Do the mmap */
void *do_mmap(void *addr, size_t length, int writable,
			  struct file *file, off_t offset)
{
	// printf("\nmmap 시작 \n");
	size_t read_bytes = length;
	void *address = addr;

	// lock_acquire(&filesys_lock);
	struct file *reopen_file = file_reopen(file);
	// lock_release(&filesys_lock);
	while (read_bytes > 0)
	{
		// printf("do_mmap ||| addr : %p\n", address);
		// printf("do_mmap ||| read_bytes : %d\n", read_bytes);
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct auxillary *tmp = malloc(sizeof(struct auxillary));
		tmp->file = reopen_file; // 파일이 close돼도 mmap이 연결되어 있어야 함 (연결 끊을 때 쓸 file정보)
		tmp->prb = page_read_bytes;
		tmp->pzb = page_zero_bytes;
		tmp->ofs = offset;
		tmp->start_address = addr;

		if (!vm_alloc_page_with_initializer(VM_FILE, address, writable, lazy_load_contents, tmp))
			return NULL;

		/* Advance. */
		// length -= PGSIZE;
		offset += page_read_bytes;
		read_bytes -= page_read_bytes;
		address += PGSIZE;
	}
	// printf("spt에서 찾은 결과 : %p\n", spt_find_page(&thread_current()->spt, addr));
	return addr;
}

/* Do the munmap */
// void do_munmap(void *addr)
// {
// 	// printf("파일 다시쓰기 성공?\n");

// 	struct page *page;

// 	void *check_addr = addr;
// 	// page != null
// 	while (page = spt_find_page(&thread_current()->spt, check_addr))
// 	{
// 		// printf("파일 다시쓰기 성공22222?\n");

// 		// 파일의 전체 페이지인지 순회하면서 체크
// 		// printf("start_address:%p addr:%p\n", page->start_address, addr);
// 		if (page->start_address == addr)
// 		{
// 			// printf("파일 다시쓰기 성공333333333?\n");
// 			page->start_address = NULL;
// 			// printf("언맵이 돌아가나?\n");
// 			// 페이지 쓰기 여부 확인후 파일에 다시 쓰기
// 			if (page->frame != NULL)
// 			{
// 				// printf("파일 다시쓰기 444444444444?\n");
// 				// 페이지가 dirty (true)하면 = 쓰기를 했으면
// 				if (pml4_is_dirty(thread_current()->pml4, check_addr))
// 				{
// 					// printf("파일 다시쓰기 성공?????????????????????\n");

// 					// 파일에 다시 써준다.
// 					struct auxillary *auxi = page->file.aux;
// 					file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
// 					// printf("파일 다시쓰기 성공?\n");
// 				}
// 			}

// 			// spt에서 프레임 반환 및 페이지 삭제
// 			spt_remove_page(&thread_current()->spt, page);
// 		}
// 		check_addr += PGSIZE;
// 	}
// }
void do_munmap(void *addr)
{
	// printf("do_munmap시작 addr : %p \n", addr);
	struct page *page;
	void *s_addr = page->start_address;

	// lock_acquire(&filesys_lock);
	while (page = spt_find_page(&thread_current()->spt, addr))
	{

		/* 페이지의 시작주소가 다르면 다른 파일이다 */
		if (s_addr != page->start_address)
			break;

		page->start_address = NULL;

		/* 프레임이 할당되었고 수정되었으면 파일에 적음 */
		if (page->frame != NULL && pml4_is_dirty(thread_current()->pml4, addr))
		{
			struct auxillary *auxi = page->file.aux;
			// lock_acquire(&filesys_lock);
			file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
			// lock_release(&filesys_lock);
		}
		spt_remove_page(&thread_current()->spt, page);
		addr += PGSIZE;
	}
	// lock_release(&filesys_lock);
}
