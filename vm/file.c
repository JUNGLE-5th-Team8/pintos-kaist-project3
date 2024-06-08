/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "string.h"
#include "threads/mmu.h"

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
	free(file_page->aux);
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
	file_seek(auxi->file, auxi->ofs);

	/* 할당된 페이지에 로드 */
	off_t result = file_read(auxi->file, page->frame->kva, auxi->prb);
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
// void *do_mmap(void *addr, size_t length, int writable, struct file *file, off_t offset)
// {

// 	// 페이지 채우기
// 	while (length > 0)
// 	{
// 		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
// 		size_t page_zero_bytes = PGSIZE - page_read_bytes;

// 		/* Set up aux to pass information to the lazy_load_segment. */
// 		void *aux = NULL;
// 		struct auxillary *aux_info = malloc(sizeof(struct auxillary));

// 		aux_info->file = file_reopen(file); // 매핑된 파일 살리기 위해 구조체 재설정해줌.
// 		aux_info->ofs = offset;
// 		aux_info->prb = page_read_bytes;
// 		aux_info->pzb = page_zero_bytes;
// 		aux_info->start_address = addr;

// 		aux = aux_info;
// 		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_contents, aux))
// 		{
// 			return NULL;
// 		}

// 		// 다음 페이지로 이동
// 		offset += page_read_bytes;
// 		length -= page_read_bytes;
// 		addr += PGSIZE;
// 	}

// 	return addr;
// }
void *do_mmap(void *addr, size_t length, int writable,
			  struct file *file, off_t offset)
{
	size_t read_bytes = length;
	void *address = addr;

	while (read_bytes > 0)
	{
		// printf("do_mmap ||| addr : %p\n", address);
		// printf("do_mmap ||| read_bytes : %d\n", read_bytes);
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct auxillary *tmp = malloc(sizeof(struct auxillary));
		tmp->file = file_reopen(file); // 파일이 close돼도 mmap이 연결되어 있어야 함 (연결 끊을 때 쓸 file정보)
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
	return addr;
}
/* Do the munmap */
// void do_munmap(void *addr)
// {
// 	struct page *page;

// 	void *check_addr;
// 	// page != null
// 	while (page = spt_find_page(&thread_current()->spt, check_addr))
// 	{
// 		// 파일의 전체 페이지인지 순회하면서 체크
// 		if (page->start_address == addr)
// 		{
// 			// printf("언맵이 돌아가나?\n");
// 			// 페이지 쓰기 여부 확인후 파일에 다시 쓰기
// 			if (page->frame == NULL)
// 			{
// 				// 페이지가 dirty (true)하면 = 쓰기를 했으면
// 				if (pml4_is_dirty(&thread_current()->pml4, check_addr))
// 				{
// 					// 파일에 다시 써준다.
// 					struct auxillary *auxi = page->file.aux;
// 					file_write_at(auxi->file, check_addr, auxi->prb, auxi->ofs);
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
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	void *s_addr = page->start_address;

	while (page = spt_find_page(&thread_current()->spt, addr))
	{

		/* 페이지의 시작주소가 다르면 다른 파일이다 */
		if (page == NULL || s_addr != page->start_address)
			break;

		// page->start_address = NULL;

		/* 프레임이 할당되었고 수정되었으면 파일에 적음 */
		if (page->frame != NULL && pml4_is_dirty(thread_current()->pml4, addr))
		{
			struct auxillary *auxi = page->file.aux;
			file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
		}

		struct auxillary *auxi = page->file.aux;
		file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
		spt_remove_page(&thread_current()->spt, page);
		addr += PGSIZE;
	}
}
