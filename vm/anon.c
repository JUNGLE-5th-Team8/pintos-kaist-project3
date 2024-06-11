/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/mmu.h"
#include "threads/malloc.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

struct swap_table st;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};
// disk_size(swap_disk)
/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	//파일을 OPEN하면 되지 않을까?
	swap_disk = disk_get(1,1);
	st.sb = bitmap_create(disk_size(swap_disk));
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &anon_ops;
	page->anon.type = type;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	struct anon_page *anon_page = &page->anon;
	disk_read(swap_disk,anon_page->st_idx,kva);
	// printf("%d!!!!!!!\n",page->anon.st_idx);
	bitmap_reset(st.sb,page->anon.st_idx);
	// printf("%d!!!!!!!\n",page->anon.st_idx);
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	disk_sector_t idx = bitmap_scan_and_flip(st.sb,0,1,false);
	// printf("!!!!idx%d\n",idx);
	if(idx==BITMAP_ERROR){
		printf("????????\n");
		return false;
	}
	disk_write(swap_disk,idx,page->frame->kva);
	anon_page->st_idx = idx;

	list_remove(&page->ft_elem);
	pml4_clear_page(page->thread->pml4,page->va);
	palloc_free_page(page->frame->kva);
	// free(page->frame);
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
}
