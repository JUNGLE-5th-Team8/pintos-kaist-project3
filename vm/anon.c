/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "lib/kernel/bitmap.h"
#include "lib/kernel/list.h"
#include "include/threads/mmu.h"
#include "threads/malloc.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

struct swap_table swap_table; // 스왑테이블 전역변수 선언

/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);							 // 스왑영역 할당
	swap_table.sb = bitmap_create(disk_size(swap_disk)); // 스왑테이블 비트맵 할당
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &anon_ops;
	page->anon.type = VM_ANON;
	struct anon_page *anon_page = &page->anon;

	/*----swaping------*/
	// page->sw_idx = -1; // 스왑 슬롯 인덱스 초기화

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	struct anon_page *anon_page = &page->anon;
	disk_read(swap_disk, page->sw_idx, kva);
	bitmap_reset(swap_table.sb, page->sw_idx);
	printf("스왑인이 된다고?\n"); // debug

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	struct anon_page *anon_page = &page->anon;

	// 여유 swap slot 탐색 = bitmap을 first-fit 알고리즘을 이용하여 탐색
	disk_sector_t idx = bitmap_scan_and_flip(swap_table.sb, 0, 1, false);
	if (idx == BITMAP_ERROR)
	{
		PANIC("스왑아웃 실패인가?\n");
		// printf("스왑 슬롯 찾기 실패!!\n"); // debug
		return false;
	}

	// 디스크에 쓰기
	disk_write(swap_disk, idx, page->frame->kva);

	// pml4에서 엔트리 삭제하기
	pml4_clear_page(page->thread->pml4, page->va);

	printf("anon swap out: %p\n", page->frame->kva);
	// 물리 메모리 해제
	palloc_free_page(page->frame->kva);
	free(page->frame);

	printf("anon swap out222222: %p\n", page->frame);

	// 프레임 테이블에서 삭제
	list_remove(&page->frame_elem);

	// 페이지 스왑 슬롯 인덱스 저장
	page->sw_idx = idx;

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
}
