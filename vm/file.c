/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "include/vm/vm.h"
#include "include/userprog/process.h"
#include "threads/malloc.h"
#include "include/threads/mmu.h"
#include <string.h>

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

	lazy_load_info *aux = malloc(sizeof(lazy_load_info));
	memcpy(aux, page->uninit.aux, sizeof(lazy_load_info));

	struct file_page *file_page = &page->file;

	/* 파일을 어디에 쓸지에 대한 정보 필요*/
	// aux_info를 uninit에서 복사
	file_page->type = VM_FILE;
	file_page->aux = aux;

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
	// aux->file = close
	// aux = free
	// file_close(file_page->aux->file);
	if (page->frame != NULL)
	{

		// 페이지가 dirty (true)하면 = 쓰기를 했으면
		if (pml4_is_dirty(thread_current()->pml4, page->start_address))
		{
			lazy_load_info *aux = page->file.aux;

			bool flag = false;
			// merger test lock
			if (!lock_held_by_current_thread(&filesys_lock))
			{
				lock_acquire(&filesys_lock);
				flag = true;
			}
			file_write_at(aux->file, page->start_address, aux->read_bytes, aux->offset);
			if (flag)
			{
				flag = false;
			}
			lock_release(&filesys_lock);
		}
	}

	free(file_page->aux);
}

// 파일을 읽어서 메모리에 콘텐츠를 적재하는 함수
static bool
lazy_load_contents(struct page *page, void *aux)
{
	if (page == NULL)
	{
		return false;
	}

	lazy_load_info *info = aux;

	page->is_loaded = true;

	bool flag = false;
	// merger test lock
	if (!lock_held_by_current_thread(&filesys_lock))
	{
		lock_acquire(&filesys_lock);
		flag = true;
	}
	file_seek(info->file, info->offset);
	if (flag)
	{
		flag = false;
	}
	lock_release(&filesys_lock);

	// merger test lock
	if (!lock_held_by_current_thread(&filesys_lock))
	{
		lock_acquire(&filesys_lock);
		flag = true;
		// 파일에서 페이지를 읽어 메모리에 로드한다.
	}
	if (file_read(info->file, page->frame->kva, info->read_bytes) != (int)info->read_bytes)
	{
		if (flag)
		{
			flag = false;
		}
		lock_release(&filesys_lock);
		return false; // 파일 읽기 실패
	}

	if (flag)
	{
		flag = false;
	}
	lock_release(&filesys_lock);

	memset(page->frame->kva + info->read_bytes, 0, info->zero_bytes);

	// free(info); // unchecked aux malloc free/

	return true;
}

/* Do the mmap */
void *do_mmap(void *addr, size_t length, int writable, struct file *file, off_t offset)
{

	void *check_addr = addr;

	bool flag = false;
	// merger test lock
	if (!lock_held_by_current_thread(&filesys_lock))
	{
		lock_acquire(&filesys_lock);
		flag = true;
	}
	file = file_reopen(file);
	if (flag)
	{
		flag = false;
	}
	lock_release(&filesys_lock);

	// 페이지 채우기
	while (length > 0)
	{
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Set up aux to pass information to the lazy_load_segment. */
		void *aux = NULL;
		lazy_load_info *aux_info = malloc(sizeof(struct lazy_load_info_t));

		aux_info->file = file; // 매핑된 파일 살리기 위해 구조체 재설정해줌.
		aux_info->offset = offset;
		aux_info->read_bytes = page_read_bytes;
		aux_info->zero_bytes = page_zero_bytes;
		aux_info->start_addr = check_addr;

		aux = aux_info;
		if (!vm_alloc_page_with_initializer(VM_FILE, check_addr, writable, lazy_load_contents, aux))
		{
			return NULL;
		}

		// 다음 페이지로 이동
		offset += page_read_bytes;
		length -= page_read_bytes;
		check_addr += PGSIZE;
	}

	return addr;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	struct page *page = NULL;

	void *check_addr = addr;
	page = spt_find_page(&thread_current()->spt, addr);

	// page != null
	while (page = spt_find_page(&thread_current()->spt, check_addr))
	{
		// 파일의 전체 페이지인지 순회하면서 체크
		if (page->start_address == addr)
		{
			page->start_address = NULL;
			// 페이지 쓰기 여부 확인후 파일에 다시 쓰기
			if (page->frame != NULL)
			{
				// 페이지가 dirty (true)하면 = 쓰기를 했으면
				if (pml4_is_dirty(thread_current()->pml4, check_addr))
				{
					lazy_load_info *aux = page->file.aux;

					bool flag = false;
					// merger test lock
					if (!lock_held_by_current_thread(&filesys_lock))
					{
						lock_acquire(&filesys_lock);
						flag = true;
					}
					file_write_at(aux->file, check_addr, aux->read_bytes, aux->offset);

					if (flag)
					{
						flag = false;
					}
					lock_release(&filesys_lock);
				}
			}

			// spt에서 프레임 반환 및 페이지 삭제
			spt_remove_page(&thread_current()->spt, page);
		}

		check_addr += PGSIZE;
	}
}
