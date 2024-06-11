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
	// printf("롸이터블?? %d 어드레스 %p\n",page->writable,page->va);

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	struct anon_page *anon_page = &page->anon;
	
	bitmap_set_multiple(st.sb,page->anon.st_idx,8,false);
	for(int i=0;i<8;i++){
		disk_read(swap_disk,anon_page->st_idx+i,(char*)(page->frame->kva)+DISK_SECTOR_SIZE*i);
	}

	// printf("%d!!!!!!!\n",page->anon.st_idx);
	// printf("인덱스 %d 어드레스 %p 롸이터블? %d!!!!!!!\n",page->anon.st_idx,page->va,page->writable);
	page->is_loaded = true;

	pml4_set_page(page->thread->pml4, page->va, kva, true);

	anon_page->st_idx = -1;
	// page->writable =true;
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	// printf(" %p 야발ㅋㅋㅋ\n",page->va);
	struct anon_page *anon_page = &page->anon;
	size_t idx = bitmap_scan_and_flip(st.sb,0,8,false);
	// printf("!!!!idx%d\n",idx);
	if(idx==BITMAP_ERROR){
		printf("????????\n");
		return false;
	}
	// disk_write(swap_disk,id)
	for(int i=0;i<8;i++){
		disk_write(swap_disk,idx+i,(char*)(page->frame->kva)+DISK_SECTOR_SIZE*i);
	}
	anon_page->st_idx = idx;

	list_remove(&page->ft_elem);
	pml4_clear_page(page->thread->pml4,page->va);
	// palloc_free_page(page->frame->kva);

	page->is_loaded = false;
	// page->frame->page = NULL;
	// page->frame = NULL;
	// page->writable =true;
	// free(page->frame);
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
}
