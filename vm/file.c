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
	file_page->type = type;
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
	// file_close(((struct auxillary *)(file_page->aux))->file);
	free(file_page->aux);
}

static bool
lazy_load_contents(struct page *page, void *aux)
{
	/* 페이지 확인 */
	if (page == NULL)
	{
		// printf("page 가 null임\n"); /* Debug */
		return false;
	}

	/* aux로 전달받은 변수 */
	struct auxillary *auxi = aux;
	struct file *file = auxi->file;
	size_t page_read_bytes = auxi->prb;
	size_t page_zero_bytes = auxi->pzb;
	off_t ofs = auxi->ofs;
	// printf("page_read_bytes : %d\n", page_read_bytes);
	// printf("page_zero_bytes : %d\n", page_zero_bytes);

	file_seek(file, ofs);
	// printf("ofs : %d\n", ofs);
	/* 할당된 페이지에 로드 */
	off_t result = file_read(file, page->frame->kva, page_read_bytes);
	// printf("result : %d\n", result);
	if (result != (off_t)page_read_bytes)
	{
		// printf("file_read 실패 \n"); /* Debug */
		return false;
	}

	/* 페이지의 남은 부분을 0으로 채우기 */
	memset(page->frame->kva + page_read_bytes, 0, page_zero_bytes);

	/* aux 동적할당 해제 */
	// printf("load 완료\n"); /* Debug */
	free(aux);
	return true;
}

/* Do the mmap */
void *do_mmap(void *addr, size_t length, int writable,
			  struct file *file, off_t offset)
{
	size_t read_bytes = length;
	uint64_t *address = addr;

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
		read_bytes -= page_read_bytes;
		offset += page_read_bytes;
		address += PGSIZE;
	}
	return addr;
}
/* Do the munmap */
void do_munmap(void *addr)
{
	// printf("do_munmap시작 addr : %p \n", addr);
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	void *s_addr = page->start_address;

	while (page = spt_find_page(&thread_current()->spt, addr))
	{
		/* 페이지의 시작주소가 다르면 다른 파일이다 */
		if (s_addr == page->start_address)
			break;

		/* 아직 프레임이 할당되지 않았으면 spt에서 삭제하고 넘어감 */
		if (page->frame == NULL)
		{
			// printf("할당되지 않은 페이지\n");
			spt_remove_page(&thread_current()->spt, page);
			addr += PGSIZE;
			continue;
		}

		/* 수정되지 않았으면 spt에서 삭제하고, frame free하고 넘어감 */
		if (!pml4_is_dirty(thread_current()->pml4, addr))
		{
			// printf("수정되지 않은 페이지\n");
			spt_remove_page(&thread_current()->spt, page);
			addr += PGSIZE;
			continue;
		}

		// /* 프레임이 할당되었고 수정되었으면 파일에 적P음 */
		// if (page->frame != NULL && pml4_is_dirty(thread_current()->pml4, addr))
		// {
		// 	struct auxillary *auxi = page->file.aux;
		// 	file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
		// }
		// printf("할당 + 수정\n");
		struct auxillary *auxi = page->file.aux;
		file_write_at(auxi->file, addr, auxi->prb, auxi->ofs);
		free(page->frame);
		spt_remove_page(&thread_current()->spt, page);
		addr += PGSIZE;
	}
}
